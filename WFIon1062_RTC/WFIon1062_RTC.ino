// this is called every 0.5s from the RTC
// https://forum.pjrc.com/threads/73076-Generating-Approximating-a-PPS-signal-from-Teensy-4-1
volatile uint32_t cbCnt = 0;
void callback() {
  cbCnt = ARM_DWT_CYCCNT;
  SNVS_HPSR |= 0b11;               // reset interrupt flag
  asm("dsb");                      // wait until flag is synced over the busses to prevent double calls of the isr
}

// sets up the RTC to generate a 2Hz interrupt.
void setupRTCInterrupt() {
  // disable periodic interrupt
  SNVS_HPCR &= ~SNVS_HPCR_PI_EN;
  while ((SNVS_HPCR & SNVS_HPCR_PI_EN)) {}  //The RTC is a slow peripheral, need to spin until PI_EN is reset...

  // set interrupt frequency to 2Hz / 0.5s
  SNVS_HPCR = SNVS_HPCR_PI_FREQ(0b1110);  // change to 0b1111 if you need a 1s interrupt

  // enable periodic interrupt
  SNVS_HPCR |= SNVS_HPCR_PI_EN;
  while (!(SNVS_HPCR & SNVS_HPCR_PI_EN)) {}  // spin until PI_EN is set...

  // attach a callback
  attachInterruptVector(IRQ_SNVS_IRQ, callback);
  NVIC_SET_PRIORITY(IRQ_SNVS_IRQ, 16);  // some priority 0-255 (low number = high priority)
  NVIC_ENABLE_IRQ(IRQ_SNVS_IRQ);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  setupRTCInterrupt();
  Serial.begin(22);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
}

extern "C" int _gettimeofday(struct timeval *tv, void *ignore __attribute__((unused)));
void loop() {
  static uint32_t ccLast = 0;
  static uint32_t cbLast = 0;
  static uint32_t usLast = 0;
  digitalToggleFast(LED_BUILTIN);  // toggle a pin to generate the pps signal
  if (cbLast != cbCnt) {
    struct timeval tv;
    int aV;
    uint32_t cc = ARM_DWT_CYCCNT;
    time_t t = rtc_get();  // get current time in seconds after 1970-01-01
    uint32_t us = micros();
    Serial.print(cc - cbCnt);  // show cycle count diff ref the callback
    Serial.print(" CB cycles\t");
    Serial.print(cc - ccLast);  // show cycle count diff per RTC second
    Serial.print(" cycles\t");
    Serial.print(us - usLast);  // show micros() diff per RTC second
    Serial.print(" us  \t");
    Serial.print(ctime(&t));  // pretty print the time
    _gettimeofday(&tv, &aV);
    Serial.print("\t");
    Serial.print(tv.tv_sec % 60);
    Serial.print('.');
    Serial.println(tv.tv_usec);
    ccLast = cc;
    usLast = us;
    cbLast = cbCnt;
    delayMicroseconds(100); // USB print interrupts will wake WFI so delay until print complete
  }
  asm volatile( "wfi" );  // WFI instruction will start entry into STOP mode
}

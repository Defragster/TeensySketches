volatile uint32_t cbCnt = 0;
uint32_t lastFreq = 15;
uint32_t cntFreq = 0;
void callback() {
  cbCnt = ARM_DWT_CYCCNT;
  SNVS_HPSR |= 0b11;               // reset interrupt flag
  asm("dsb");                      // wait until flag is synced over the busses to prevent double calls of the isr
}

// sets up the RTC to generate interrupt.
// page 1248: 7-4 PI_FREQ : Periodic Interrupt Frequency values 0-15
void setupRTCInterrupt( uint8_t valPIF ) {
  static bool cbSet = false;
  cntFreq = 0;
  Serial.printf( "\n\tsetupRTCInterrupt PIF set to %u 0x%X\n", valPIF, valPIF );
  // disable periodic interrupt
  SNVS_HPCR &= ~SNVS_HPCR_PI_EN;
  while ((SNVS_HPCR & SNVS_HPCR_PI_EN)) {}  //The RTC is a slow peripheral, need to spin until PI_EN is reset...
  SNVS_HPCR = SNVS_HPCR_PI_FREQ( valPIF );  // change to 0b1111 if you need a 1s interrupt
  // enable periodic interrupt
  SNVS_HPCR |= SNVS_HPCR_PI_EN;
  while (!(SNVS_HPCR & SNVS_HPCR_PI_EN)) {}  // spin until PI_EN is set...

  // attach a callback - doesn't change after first call
  if (!cbSet) {
    cbSet = true;
    attachInterruptVector(IRQ_SNVS_IRQ, callback);
    NVIC_SET_PRIORITY(IRQ_SNVS_IRQ, 16);  // some priority 0-255 (low number = high priority)
    NVIC_ENABLE_IRQ(IRQ_SNVS_IRQ);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(22);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  setupRTCInterrupt( lastFreq );
}

extern "C" int _gettimeofday(struct timeval *tv, void *ignore __attribute__((unused)));
void loop() {
  static uint32_t usLast = 0;
  static uint32_t ccLast = 0;
  digitalToggleFast(LED_BUILTIN);  // toggle a pin to generate the pps signal
  if (ccLast != cbCnt) {
    uint32_t us = micros();
    if ( 0 != cntFreq ) {
      //      Serial.print(cc - ccLast - ( cc - cbCnt ) );  // show cycle count diff ref the callback
      Serial.print(cbCnt - ccLast );  // show cycle count diff ref the callback
      Serial.print(" RTC ISR cycles\t");
      Serial.print(us - usLast);  // show micros() diff per RTC second
      Serial.print(" us diff\t");
      Serial.print(F_CPU_ACTUAL / (cbCnt - ccLast));  //
      Serial.println(" Hz");
    }
    usLast = us;
    ccLast = cbCnt;
    cntFreq++;
    if (cntFreq > 3) {
      if ( lastFreq >= 1 )
        lastFreq--;
      else {
        lastFreq = 15;
        setupRTCInterrupt( lastFreq ); // Run SLOW to wake WFI less often
        while (1) {
          asm volatile( "wfi" ); // LOW POWER STALL
        }
      }
      setupRTCInterrupt( lastFreq );
    }
  }
}

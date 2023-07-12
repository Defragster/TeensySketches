void RTCcallback() {
  SNVS_HPSR |= 0b11;               // reset interrupt flag
  digitalToggleFast(LED_BUILTIN);
  asm("dsb");                      // wait until flag is synced over the busses to prevent double calls of the isr
}

// valPIF given divisor of 32Khz of 15 - 0 = Hz: 15 = 1, 14 = 2, 13 = 4, ..., 3 = 4096, 2 = 8192, 1 = 16384, 0 = 32772
void setupRTCInterrupt( uint8_t valPIF ) {
  SNVS_HPCR &= ~SNVS_HPCR_PI_EN; // disable periodic interrupt
  while ((SNVS_HPCR & SNVS_HPCR_PI_EN)) {}  //The RTC is a slow peripheral, need to spin until PI_EN is reset...
  SNVS_HPCR = SNVS_HPCR_PI_FREQ( valPIF );
  SNVS_HPCR |= SNVS_HPCR_PI_EN; // enable periodic interrupt
  while (!(SNVS_HPCR & SNVS_HPCR_PI_EN)) {}  // spin until PI_EN is set...
  attachInterruptVector(IRQ_SNVS_IRQ, RTCcallback);
  NVIC_SET_PRIORITY(IRQ_SNVS_IRQ, 16);  // some priority 0-255 (low number = high priority)
  NVIC_ENABLE_IRQ(IRQ_SNVS_IRQ);
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  setupRTCInterrupt( 12 ); // 14 is 1 Hz
  Serial.begin(22);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
    Serial.printf( "Millis=%lu deg  C=%2.2f ", millis(), tempmonGetTemp() );
}

struct timeval tv;
int aV;

uint32_t lCnt = 0;
void loop()
{
  static uint32_t lMillis = millis();
  lCnt++;
  if ( lCnt < 20 )
    Serial.printf( "Millis=%lu [ms diff=%lu] Micros=%lu deg  C=%2.2f RTC time=%s", millis(), millis() - lMillis , micros(), tempmonGetTemp(), ctime(&tv.tv_sec) );
  else
    lCnt = 30;
  delayMicroseconds(100);
  lMillis = millis();
  asm volatile( "wfi" ); // LOW POWER STALL
  if ( lCnt > 10 ) {
    resetMMclocks();
  }
}

extern "C" int _gettimeofday(struct timeval * tv, void *ignore __attribute__((unused)));
extern volatile uint32_t systick_millis_count;
extern volatile uint32_t systick_cycle_count;
void resetMMclocks() {
  _gettimeofday(&tv, &aV);
  __disable_irq();
  systick_millis_count = 1000 * (tv.tv_sec % 86400); // one day back
  systick_cycle_count = ARM_DWT_CYCCNT - F_CPU_ACTUAL * 1000.0/tv.tv_usec;
  __enable_irq();
}

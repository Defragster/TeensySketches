// update per p#20:: https://forum.pjrc.com/threads/60315-quot-wfi-quot-on-Teensy-4-0-stops-millis()?p=329138&viewfull=1#post329138
uint32_t isrCnt = 0;
void RTCcallback() {
  SNVS_HPSR |= 0b11;               // reset interrupt flag
  digitalToggleFast(LED_BUILTIN);
  isrCnt++;
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
  setupRTCInterrupt( 15 ); // 14 is 1 Hz
  Serial.begin(22);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  Serial.printf( "Millis=%lu deg  C=%2.2f \n", millis(), tempmonGetTemp() );
  resetMMclocks();
  Serial.printf( "Millis=%lu deg  C=%2.2f \n", millis(), tempmonGetTemp() );
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
  delayMicroseconds(10);
  lMillis = millis();
  asm volatile( "wfi" ); // LOW POWER STALL
  if ( lCnt > 10 ) {
    resetMMclocks();
  }
  if ( isrCnt > 20 ) {
    lCnt = 0;
    isrCnt = 0;
    Serial.println();
  }
}

extern "C" int _gettimeofday(struct timeval * tv, void *ignore __attribute__((unused)));
extern volatile uint32_t systick_millis_count;
extern volatile uint32_t systick_cycle_count;
void resetMMclocks() {
  _gettimeofday(&tv, &aV);
  uint64_t ms1970;;
  __disable_irq();
  ms1970 = 1000 * tv.tv_sec + tv.tv_usec / 1000;
  systick_millis_count = ms1970;
  systick_cycle_count = ARM_DWT_CYCCNT - F_CPU_ACTUAL * 1000000.0 / tv.tv_usec;
  __enable_irq();
}

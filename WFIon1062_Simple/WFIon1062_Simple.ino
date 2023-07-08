void callback() {
  SNVS_HPSR |= 0b11;               // reset interrupt flag
  digitalToggleFast(LED_BUILTIN);
  asm("dsb");                      // wait until flag is synced over the busses to prevent double calls of the isr
}

void setupRTCInterrupt( uint8_t valPIF ) {
  // disable periodic interrupt
  SNVS_HPCR &= ~SNVS_HPCR_PI_EN;
  while ((SNVS_HPCR & SNVS_HPCR_PI_EN)) {}  //The RTC is a slow peripheral, need to spin until PI_EN is reset...
  SNVS_HPCR = SNVS_HPCR_PI_FREQ( valPIF );  // change to 0b1111 if you need a 1s interrupt
  // enable periodic interrupt
  SNVS_HPCR |= SNVS_HPCR_PI_EN;
  while (!(SNVS_HPCR & SNVS_HPCR_PI_EN)) {}  // spin until PI_EN is set...
  attachInterruptVector(IRQ_SNVS_IRQ, callback);
  NVIC_SET_PRIORITY(IRQ_SNVS_IRQ, 16);  // some priority 0-255 (low number = high priority)
  NVIC_ENABLE_IRQ(IRQ_SNVS_IRQ);
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  setupRTCInterrupt( 14 ); // 14 is 2 Hz
}

void loop()
{
  asm volatile( "wfi" ); // LOW POWER STALL
}

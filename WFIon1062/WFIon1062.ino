void setup() {
  Serial.begin(9600);
  while (!Serial);  // wait for serial connect
  if (CrashReport) Serial.print( CrashReport );
  Serial.println("\nTeensy 4.x WFI for lower Temp/Power");
}

extern "C" uint32_t set_arm_clock(uint32_t frequency);
#include "intervaltimer.h"
IntervalTimer Alpha;
void testAlpha() {
  Alpha.end(); // stop the timer
}
bool doneOnce = false;
void loop() {
#if 1 // DEBUG show periodic wake and temp to monitor
  pinMode( 13, OUTPUT );
  digitalToggle( 13 );
  Serial.printf( "   deg  C=%2.2f  ms=%lu\n" , tempmonGetTemp(), millis() );
  delayMicroseconds(100); // USB print interrupts will wake WFI so delay until print complete
#endif
  if ( F_CPU_ACTUAL > 30000000 ) // T_4.0 at 35C==95F w/WFI
    set_arm_clock(24000000);
#if 1 // To wake from WFI require an explicit interrupt
#if 0 // To wake from WFI require an intervaltimer interrupt
  if ( !doneOnce ) {
    Serial.println( "   wake with intervaltimer" );
    delayMicroseconds(100);
    doneOnce = true;
  }
  Alpha.begin( testAlpha, 2000000 ); // T_4.0 at 45C==113F
  asm volatile( "wfi" );  // WFI instruction will start entry into STOP mode
  Serial.print( "iT:" );
#else
  if ( !doneOnce ) {
    Serial.println( "   wake with SYS_TICK as real ISR" );
    delayMicroseconds(100);
    doneOnce = true;
    // keep memory powered during sleep
    CCM_CGPR |= CCM_CGPR_INT_MEM_CLK_LPM;
    // keep cpu clock on in wait mode (required for systick to trigger wake-up)
    CCM_CLPCR &= ~(CCM_CLPCR_ARM_CLK_DIS_ON_LPM | CCM_CLPCR_LPM(3));
    // set SoC low power mode to wait mode
    CCM_CLPCR |= CCM_CLPCR_LPM(1);
    // ensure above config is done before executing WFI
    asm volatile("dsb");
  }
  asm volatile( "wfi" );  // WFI instruction will start entry into STOP mode
  Serial.print( "ST:" );
#endif
#else
  delay(200);  // T_4.0 at 60C==140F
#endif
}

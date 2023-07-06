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
void loop() {
#if 1 // DEBUG show periodic wake and temp to monitor
  pinMode( 13, OUTPUT );
  digitalToggle( 13 );
  Serial.printf( "   deg  C=%2.2f\n" , tempmonGetTemp() );
  delay(2); // USB print interrupts will wake WFI so delay until print complete
#endif
  if ( F_CPU_ACTUAL > 30000000 ) // T_4.0 at 35C==95F w/WFI
    set_arm_clock(24000000);
#if 1 // To wake from WFI require an explicit interrupt
  Alpha.begin( testAlpha, 2000000 ); // T_4.0 at 45C==113F
  asm volatile( "wfi" );  // WFI instruction will start entry into STOP mode
#else
  delay(200);  // T_4.0 at 60C==140F
#endif
}

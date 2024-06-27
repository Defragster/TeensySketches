// Requires Teensy with DUAL USB Serial
// ESP Serial echos to USB and COMMAND 'r' or 'p' issued to USB also
// USB1 tied to proxy of ESP32 UART. Can watch for ECHO, must disconnect to Proxy Upload!
int led = 13;
int resetPin = 3;  // PIN connected to ESP32 RESET
int pin0 = 2;   // PIN connected to ESP32 GPIO 0

void setup() {
  pinMode(led, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(pin0, OUTPUT);
  digitalWrite(resetPin, HIGH);
  digitalWrite(pin0, HIGH);
  Serial1.begin(115200);
  Serial.begin(115200);
  SerialUSB1.begin(115200);
  Serial.print( "\nSerial MAIN HERE : Commnad 'r' or 'p'\n" );
  SerialUSB1.print( "\nSerialUSB1: ESP32 SerMon. Disconnect to Program\n" );
}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  while ( Serial.available() ) { // Monitor for reset and Program commnands
    char inCH = Serial.read();
    if ( inCH == 'p') {
      Serial.print( "Setting to Program mode ..." );
      SetProgram( );
      Serial.print( " Done.\n" );
    }
    if ( inCH == 'r') {
      Serial.print( "Retting ESP32 ..." );
      ESPReset( );
      Serial.print( " Done.\n" );
    }
  }
  while ( Serial1.available() ) { // Monitor for ESP32 UART output
    char inCH = Serial1.read();
    SerialUSB1.print( inCH ); // Feedback to SerMon or Programing computer
    Serial.print( inCH ); // Copy to USB SerMon
  }
#define ARR_MOD 16 // show code upload data groups as it progresses
  static int iCnt = 0;
  static char chIn[ARR_MOD + 4];
  while ( SerialUSB1.available() ) {
    char inCH = SerialUSB1.read();
    Serial1.print( inCH );
    chIn[iCnt % ARR_MOD] = inCH;
    if ( !(iCnt % ARR_MOD) )     {
      Serial.printf( "\n%d\t", iCnt );
      for ( int jj = 0; jj < ARR_MOD; jj++ )
        Serial.printf( "%2.2X ", chIn[jj] );
    }
    iCnt++;
  }
}

void SetProgram() {
  digitalWrite(pin0, LOW);
  delay(500);
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  delay(5);
  digitalWrite(pin0, HIGH);
}

void ESPReset() {
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
}

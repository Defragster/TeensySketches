// Requires Teensy with DUAL USB Serial
// ESP Serial echos to USB and COMMAND 'r' or 'p' issued to USB also
// USB1 tied to proxy of ESP32 UART. Can watch for ECHO, must disconnect to Proxy Upload!
int led = 13;
int resetPin = 3;  // PIN connected to ESP32 RESET
int pin0 = 2;   // PIN connected to ESP32 GPIO 0

/* FULL PIN CONNECTION as used for WifiNina
    For Dual Proxy upload and control these are used
    Teensy Tx pin 0 to ESP32 Rx
    Teensy Rx pin 1 to ESP32 Tx
    Teensy pin 2 to ESP32 GPIO 0
    Teensy pin 3 to ESP32 Reset/Enable
    Connect a common GND
    If ESP32 not USB powered, power from Teensy Vin=5V if appropriate

    In WiFinina Example sketches
  #elif defined(TEENSYDUINO)
  #define SPIWIFI       SPI  // The SPI port
  #define SPIWIFI_SS     10   // Chip select pin
  #define ESP32_RESETN   3   // Reset pin
  #define SPIWIFI_ACK    4   // a.k.a BUSY or READY pin
  #define ESP32_GPIO0    2
*/

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
elapsedMillis showLife;
void loop() {
  UploadFunc();
  if ( showLife > 500 ) {
    showLife = 0;
    digitalToggleFast(led);
  }
}

void UploadFunc() {
  showESP();
  if ( !Serial.available() )
    return;

  char inCH;
  inCH = Serial.read();
  if ( inCH == 'r') {
    ESPReset( );
    return;
  }
  if ( inCH == 'p') {
    SetProgram( ); // Setting to Program mode
    Serial.print( "\nENTER 'r' after UPLOAD to restart ESP.\n" );
    
  }

  while ( Serial.available() ) { // Monitor for reset and Program commnands
    inCH = Serial.read();
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
      digitalToggleFast(led);
      Serial.printf( "\n%d\t", iCnt );
      for ( int jj = 0; jj < ARR_MOD; jj++ )
        Serial.printf( "%2.2X ", chIn[jj] );
    }
    iCnt++;
  }
}

void SetProgram() {
  Serial.print( "\nSetting to Program mode ..." );
  digitalWrite(pin0, LOW);
  delay(500);
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  delay(5);
  digitalWrite(pin0, HIGH);
  Serial.print( " Done.\n" );
}

void ESPReset() {
  Serial.print( "\nRetting ESP32 ..." );
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  Serial.print( " Done.\n" );
}

void   showESP() {
  while ( Serial1.available() ) { // Monitor for ESP32 UART output
    char inCH = Serial1.read();
    SerialUSB1.print( inCH ); // Feedback to SerMon or Programing computer
    Serial.print( inCH ); // Copy to USB SerMon
  }
}

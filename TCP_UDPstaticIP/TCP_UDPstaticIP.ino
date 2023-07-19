#include <QNEthernet.h>
using namespace qindesign::network;
constexpr bool kStartWithDHCP = false;
IPAddress staticIP{192, 168, 0, 150};//{192, 168, 1, 101};
IPAddress subnetMask{255, 255, 255, 0};
IPAddress gateway{192, 168, 0, 1};
IPAddress dnsServer = gateway;
// The link timeout, in milliseconds. Set to zero to not wait and
// instead rely on the listener to inform us of a link.
constexpr uint32_t kLinkTimeout = 5000;  // 5 seconds

EthernetServer server(5005);       // telnet by default, the port is used
// boolean alreadyConnected = false;  // regardless of whether the client was connected earlier or not

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 10000 ) ; // wait for serial monitor
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  if ( CrashReport ) Serial.print( CrashReport );

  uint8_t mac[6];
  Ethernet.macAddress(mac);  // This is informative; it retrieves, not sets
  printf("MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  staticIP[3] += mac[5]%15;

  if ( !kStartWithDHCP ) {
    Serial.printf("Starting Ethernet with static IP...\r\n");
    Ethernet.setDNSServerIP(dnsServer);  // Set first so that the
    // listener sees it
    Ethernet.begin(staticIP, subnetMask, gateway);

    // When setting a static IP, the address is changed immediately,
    // but the link may not be up; optionally wait for the link here
    if (kLinkTimeout > 0) {
      if (!Ethernet.waitForLink(kLinkTimeout)) {
        printf("Warning: No link detected\r\n");
        // We may still see a link later, after the timeout, so
        // continue instead of returning
      }
      else {
        IPAddress ip = Ethernet.localIP();
        if (ip != INADDR_NONE) {
          IPAddress subnet = Ethernet.subnetMask();
          Serial.printf("Local IP: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3], subnet[0], subnet[1], subnet[2], subnet[3]);
        }
      }
    }
  }
  else {
    Serial.printf("Starting Ethernet with DHCP...\r\n"); // Initialize Ethernet, in this case with DHCP
    if (!Ethernet.begin()) {
      Serial.printf("Failed to start Ethernet\r\n");
      return;
    }
  }
  Ethernet.onLinkState([](bool state) {
    Serial.printf("Link %s\r\n", state ? "ON" : "OFF");
  }); // setup callback
  Ethernet.onAddressChanged([]() {
    IPAddress ip = Ethernet.localIP();
    bool hasIP = (ip != INADDR_NONE);
    if (hasIP) {
      IPAddress subnet = Ethernet.subnetMask();
      Serial.printf("Local IP: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3], subnet[0], subnet[1], subnet[2], subnet[3]);
    } else {
      Serial.printf("Address changed: No IP address\r\n");
    }
  }); // setup callback
  //Udp.begin(localPort);
  server.begin();  // start listening to customers
  UDPsetup();
}

void loop() {
  EthernetClient client = server.available();  // we are waiting for a new client:
  if (client) {
    int iCnt = client.available();
    Serial.printf( "\n>>>>>\tclient.available chars = %d\n", iCnt );
    while (client.available() > 0) {  // reads bytes coming from the client:
      char thisChar = client.read();
      server.write(thisChar);  // pass the bytes back to the client:
      Serial.write(thisChar);
    }
    Serial.printf( "\n\tclient.available chars = %d\t<<<<<\n", iCnt );
  }
  UDPloop();
}





// SPDX-FileCopyrightText: (c) 2023 Shawn Silverman <shawn@pobox.com>
// SPDX-License-Identifier: MIT

// BroadcastChat is a simple chat application that broadcasts and
// receives text messages over UDP.
//
// This file is part of the QNEthernet library.
// defragster edit from: https://github.com/ssilverman/QNEthernet/blob/master/examples/BroadcastChat/BroadcastChat.ino

// #include <QNEthernet.h>
#include <TimeLib.h>

// using namespace qindesign::network;

// --------------------------------------------------------------------------
//  Configuration
// --------------------------------------------------------------------------

constexpr uint32_t kDHCPTimeout = 15000;  // 15 seconds
constexpr uint16_t kPort = 5190;          // Chat port

// --------------------------------------------------------------------------
//  Program State
// --------------------------------------------------------------------------

// UDP port.
EthernetUDP udp(32);  // (##)-packet buffer allocate for incoming messages

class UDPStream : public Stream {  // print and printf send UDP. print is 1KB RAM buffered until newline, flush or a printf is sent.
public:
  int printf(const char *fmt, ...) {
    va_list va;
    flush();  // send any buffered data
    va_start(va, fmt);
    char szOut[256];
    int cnt = vsnprintf(szOut, 256, fmt, va);
    va_end(va);
    udp.send(Ethernet.broadcastIP(), kPort, reinterpret_cast<const uint8_t *>(szOut), cnt);
    return cnt;
  }

  // overrides for Stream
  int available() override {
    if (tail_ == head_) tail_ = head_ = 0;
    return (tail_ - head_);
  }
  int read() override {
    return (tail_ != head_) ? buffer_[head_++] : -1;
  }
  int peek() override {
    return (tail_ != head_) ? buffer_[head_] : -1;
  }

  // overrides for Print
  size_t write(uint8_t b) override {
    int retVal = 0;
    if (tail_ < buffer_size) {
      buffer_[tail_++] = b;
      retVal = 1;
    }
    if ('\n' == b) {
      flush();
      retVal = 1;
    }
    return retVal;
  }
  void flush() override {
    if (tail_ != head_) {
      udp.send(Ethernet.broadcastIP(), kPort, reinterpret_cast<const uint8_t *>(&buffer_[head_]), tail_ - head_);
      tail_ = head_ = 0;
    }
  }

  enum { BUFFER_SIZE = 1024 };
  uint8_t buffer_[BUFFER_SIZE];
  uint32_t buffer_size = BUFFER_SIZE;
  uint32_t head_ = 0;
  uint32_t tail_ = 0;
};

UDPStream UDPdbg;

// --------------------------------------------------------------------------
//  Main Program
// --------------------------------------------------------------------------

// Forward declarations (not really needed in the Arduino environment)
static void printPrompt();
static int receivePacket();
static void sendLine();

static int ipId;
static int rPCnt = 0;
static int bootMinute;

// Program setup.
void UDPsetup() {
  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {
    // Wait for Serial
  }
  if (CrashReport) Serial.print(CrashReport);
  printf("Starting...\r\n");
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("RTC has set the system time");
  }

  uint8_t mac[6];
  Ethernet.macAddress(mac);  // This is informative; it retrieves, not sets
  printf("MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
/*
  Ethernet.onLinkState([](bool state) { // callback lambda
    printf("[Ethernet] Link %s\r\n", state ? "ON" : "OFF");
  });

  printf("Starting Ethernet with DHCP...\r\n");
  if (!Ethernet.begin()) {
    printf("Failed to start Ethernet\r\n");
    return;
  }
  if (!Ethernet.waitForLocalIP(kDHCPTimeout)) {
    printf("Failed to get IP address from DHCP\r\n");
    return;
  }
*/
  IPAddress ip = Ethernet.localIP();
  printf("    Local IP     = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  ipId = ip[3] % 60;
  printf("Starting ... ipID==%u Sec=%u\r\n", ipId, second());
  bootMinute = minute() + 3;

  ip = Ethernet.subnetMask();
  printf("    Subnet mask  = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  ip = Ethernet.broadcastIP();
  printf("    Broadcast IP = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  ip = Ethernet.gatewayIP();
  printf("    Gateway      = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  ip = Ethernet.dnsServerIP();
  printf("    DNS          = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);

  // Start UDP listening on the port
  udp.begin(kPort);
  digitalClockDisplay();
  printf("%u :: %2u:%2u:%2u %2u %2u %2u ", ipId, hour(), minute(), second(), day(), month(), year());
  // Sample UDPStream UDPdbg printing to UDP
  UDPdbg.printf("\t___%u :: %2u:%2u:%2u %2u %2u %2u ", ipId, hour(), minute(), second(), day(), month(), year());
  UDPdbg.println('b');
  UDPdbg.println(24680);
  UDPdbg.print('A');
  UDPdbg.printf("\t_2_%u :: %2u:%2u:%2u %2u %2u %2u ", ipId, hour(), minute(), second(), day(), month(), year());
  UDPdbg.print("Hello World ... do a FLUSH");
  UDPdbg.flush();
  UDPdbg.print("Hello World ... leave this in buffer a bit ..."); // No newline so this should precede first following UDPdbg as noted
  printPrompt();
}

elapsedMillis secWait;
// Main program loop.
void UDPloop() {
  int cntW = 10;
  while (receivePacket() && cntW)
    cntW--; // process up to cntW buffered messages
  sendLine();
  if (secWait > 1000) {
    static char sample[] = "1234567890 abcdefghijk LMNOPQRSTUVWXYZ";
    secWait = 0;
    // digitalClockDisplay();
    if (second() == ipId || minute() == bootMinute) {
      // UDPStream encapsulates this: udp.send(Ethernet.broadcastIP(), kPort, reinterpret_cast<const uint8_t *>(sample), sizeof(sample) - 1);
      UDPdbg.printf("%s", sample );
      uint32_t aTime = micros();
      for (int ii = 0; ii < 29; ii++) {
        UDPdbg.printf("%u :: %2u:%2u:%2u %2u %2u %2u [%u]", ipId, hour(), minute(), second(), day(), month(), year(), ii);
      }
      aTime = micros() - aTime;
      Serial.printf("\t29 UDP .send() took %lu us\n", aTime);
      printPrompt();
    }
  }
}

// --------------------------------------------------------------------------
//  Internal Functions
// --------------------------------------------------------------------------

// Control character names.
static const String kCtrlNames[]{
  "NUL",
  "SOH",
  "STX",
  "ETX",
  "EOT",
  "ENQ",
  "ACK",
  "BEL",
  "BS",
  "HT",
  "LF",
  "VT",
  "FF",
  "CR",
  "SO",
  "SI",
  "DLE",
  "DC1",
  "DC2",
  "DC3",
  "DC4",
  "NAK",
  "SYN",
  "ETB",
  "CAN",
  "EM",
  "SUB",
  "ESC",
  "FS",
  "GS",
  "RS",
  "US",
};

// Receives and prints chat packets.
static int receivePacket() {
  int size = udp.parsePacket();
  if (size < 0) {
    return 0;
  }

  // Get the packet data and remote address
  const uint8_t *data = udp.data();
  IPAddress ip = udp.remoteIP();
  rPCnt++;
  printf("[%u.%u.%u.%u][%d] ", ip[0], ip[1], ip[2], ip[3], size);

  // Print each character
  for (int i = 0; i < size; i++) {
    uint8_t b = data[i];
    if (b < 0x20) {
      printf("<%s>", kCtrlNames[b].c_str());
    } else if (b < 0x7f) {
      putchar(data[i]);
    } else {
      printf("<%02xh>", data[i]);
    }
  }
  printf("\r\n");
  return 1;
}

// Tries to read a line from the console and returns whether
// a complete line was read. This is CR/CRLF/LF EOL-aware.
static bool readLine(String &line) {
  static bool inCR = false;  // Keeps track of CR state

  while (Serial.available() > 0) {
    int c;
    switch (c = Serial.read()) {
      case '\r':
        inCR = true;
        return true;

      case '\n':
        if (inCR) {
          // Ignore the LF
          inCR = false;
          break;
        }
        return true;

      default:
        if (c < 0) {
          return false;
        }
        inCR = false;
        line.append(static_cast<char>(c));
    }
  }

  return false;
}

// Prints the chat prompt.
static void printPrompt() {
  printf("%u]chat%u> ", rPCnt, ipId);
  fflush(stdout);  // printf may be line-buffered, so ensure there's output
}

// Reads from the console and sends packets.
static void sendLine() {
  static String line;

  // Read from the console and send lines
  if (readLine(line)) {
    if (!udp.send(Ethernet.broadcastIP(), kPort,
                  reinterpret_cast<const uint8_t *>(line.c_str()),
                  line.length())) {
      printf("[Error sending]\r\n");
    }
    line = "";
    printPrompt();
  }
}

void digitalClockDisplay() {
  // digital clock display of the time
  /*Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
*/
  Serial.printf("%2u:%2u:%2u %2u %2u %2u", hour(), minute(), second(), day(), month(), year());
  Serial.println();
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER "T"  // Header tag for serial time sync message

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600;  // Jan 1 2013

  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
    if (pctime < DEFAULT_TIME) {  // check the value is a valid time (greater than Jan 1 2013)
      pctime = 0L;                // return 0 to indicate that the time is not valid
    }
  }
  return pctime;
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

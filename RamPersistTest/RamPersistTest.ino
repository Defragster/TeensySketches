// https://forum.pjrc.com/threads/66628-Using-EXTMEM-to-Place-Variables-in-External-PSRAM?p=328329&viewfull=1#post328329
#define TLEN 1024
uint8_t u8Test[TLEN];

// https://github.com/KurtE/MemoryHexDump :: T:\T_Drive\tCode\libraries\MemoryHexDump
#include <MemoryHexDump.h>
/*
  void MemoryHexDump(Print& out, void const* address, size_t count, bool remove_duplicate_lines,
	const char *szTitle=NULL, uint32_t max_output_lines=(uint32_t)-1,
	uint32_t starting_display_addr = (uint32_t)-1 );
*/

// 	DTCM (rwx):  ORIGIN = 0x20000000, LENGTH = 512K  // RAM1
//  RAM (rwx):   ORIGIN = 0x20200000, LENGTH = 512K  // RAM2 - DMAMEM
//  ERAM (rwx):  ORIGIN = 0x70000000, LENGTH = 16384K // t_4.1 psram
// Teensy 4.1 external RAM address range is 0x70000000 to 0x7FFFFFFF

// ??? #define TSize

//DMAMEM char midipromptsDM[12][8];
//EXTMEM char midipromptsEM[12][8];
extern uint8_t external_psram_size;

// TODO record block groups start and stop: do MemHexDump on the block
uint32_t iTrack[12][3];
uint32_t iTii;
char iTlabels[3][32] = { "Pre allocated X", "Overwritten/LOST -", "Static maintained +" };
void setup() {
  int ii;
  Serial.begin(9600);
  while (!Serial);  // wait for serial connect
  if (CrashReport) Serial.print( CrashReport );
  Serial.println("\n\nDRAM Persist test");
  for (ii = 0; ii < TLEN; ii++) u8Test[ii] = 'A' + ii % 16; // MHexDump tracks 16B dupes
  char *p = NULL;
  char *pKeep = NULL;
  void *pEnd;
  int tNO, tGO, tLast, tCnt;
  int pMax = 512 * 1024;
  do { // Malloc largest block of free DMAMEM/RAM2
    pMax -= 1024;
    pKeep = (char*)malloc( pMax);
    Serial.print("._");
  } while (pKeep == NULL);
  do {
    tNO = tGO = tLast = tCnt = 0;
    p = pKeep;
    Serial.printf(" Bytes %d is %d KB\n", pMax, pMax / 1024);
    pEnd = p + pMax; // (void *)(0x20200000+ 511 * 1024);
    Serial.println();
    Serial.printf("Start Address %p End at %p\n", pKeep, pEnd);
    char *pSkip = (char *)0x20200000;
    for (ii = 0; ii < 12; ii++) iTrack[ii][0] = 0;
    ii = 0;
    while ( pSkip < pKeep ) {
      Serial.print('X');
      ii++;
      pSkip += 1024;
    }
    MemoryHexDump( Serial, (void const*)0x20200000, 32, true, "Already Allocated\n", 5 );
    iTii = 0;
    iTrack[iTii][0] = 0x20200000;
    iTrack[iTii][1] = ii;
    iTrack[iTii][2] = 0;
    iTii++;
    iTrack[iTii][0] = (uint32_t)pKeep;
    iTrack[iTii][1] = 0;
    iTrack[iTii][2] = 0;
    Serial.print( ii );
    while (p < pEnd) {
      if (memcmp(u8Test, p, TLEN)) {
        tNO++;
        if ( tLast != 1 ) MemoryHexDump( Serial, (void const*)p, 1024, true, "\nOverwritten\n", 5 );
        memcpy(p, u8Test, TLEN);
        if ( tLast == 2 ) {
          Serial.print( tCnt );
          iTrack[iTii][1] = tCnt;
          iTrack[iTii][2] = 2;
          iTii++;
          iTrack[iTii][0] = (uint32_t)p;
          iTrack[iTii][2] = 1;
          tCnt = 0;
        }
        else
          iTrack[iTii][2] = 1;
        tLast = 1;
        tCnt++;
        Serial.print('-');
      } else {
        tGO++;
        if ( tLast == 1 ) {
          Serial.print( tCnt );
          iTrack[iTii][1] = tCnt;
          iTrack[iTii][2] = 1;
          iTii++;
          iTrack[iTii][0] = (uint32_t)p;
          iTrack[iTii][2] = 2;
          tCnt = 0;
        }
        else
          iTrack[iTii][2] = 2;
        tLast = 2;
        tCnt++;
        Serial.print('+');
      }
      p += 1024;
      ii++;
      if (!(ii % 50)) Serial.println(ii);
    }
    if ( 0 != tCnt ) {
      Serial.print( tCnt );
      iTrack[iTii][1] = tCnt;
    }
    arm_dcache_flush(pKeep, pMax);
    ii = 0;
    Serial.println();
    while ( 0 != iTrack[ii][0] ) {
      Serial.printf( "Start=%X Len KB=%u TYPE=%s \n", iTrack[ii][0], iTrack[ii][1], iTlabels[ iTrack[ii][2] ] );
      // MemoryHexDump( Serial, (void const*)iTrack[ii][0], iTrack[ii][1]*1024, true, "fix me\n", 5 ); // Already re-written, nothign to see
      ii++;
    }
    if ( 0 == tGO ) Serial.println("Cold Restart observed");
    else if ( 0 == tNO ) Serial.println("Memory set without any restart");
    else Serial.println();
  } while ( 0 == tGO );
  free( pKeep );
  Serial.println("END:: DRAM Persist test");
  if (external_psram_size) Serial.printf("PSRAM size %u\n", external_psram_size); // Test this too?
}

void loop() {
  if ( Serial.available() ) _reboot_Teensyduino_();
}

/*
  23:45:13.746 (loader): using encrypted ehex (required - secure mode is locked)
  DRAM Persist test
  PSRAM size 16
  ._._._._._._._._._._._._._ Bytes 510976 is 499 KB

  Start Address 0x20203088 End at 0x2027fc88
  XXXXXXXXXXXXX13+1-------------------------25+++++++++++50
  ++++++++++++++25-1+++++++++++++++++++++++++++++++++++100
  ++++++++++++++++++++++++++++++++++++++++++++++++++150
  ++++++++++++++++++++++++++++++++++++++++++++++++++200
  ++++++++++++++++++++++++++++++++++++++++++++++++++250
  ++++++++++++++++++++++++++++++++++++++++++++++++++300
  ++++++++++++++++++++++++++++++++++++++++++++++++++350
  ++++++++++++++++++++++++++++++++++++++++++++++++++400
  ++++++++++++++++++++++++++++++++++++++++++++++++++450
  ++++++++++++++++++++++++++++++++++++++++++++++++++500
  ++++++++++++
  END:: DRAM Persist test

  23:39:52.206 (loader): using hex file - Teensy not configured for encryption
  DRAM Persist test
  PSRAM size 8
  ._._._._._._._._._._._._._ Bytes 510976 is 499 KB

  Start Address 0x20203088 End at 0x2027fc88
  XXXXXXXXXXXXX13+1-------------------19+++++++++++++++++50
  ++++++++++++++++++++++++++++++++++++++++++++++++++100
  ++++++++++++++++++++++++++++++++++++++++++++++++++150
  ++++++++++++++++++++++++++++++++++++++++++++++++++200
  ++++++++++++++++++++++++++++++++++++++++++++++++++250
  ++++++++++++++++++++++++++++++++++++++++++++++++++300
  ++++++++++++++++++++++++++++++++++++++++++++++++++350
  ++++++++++++++++++++++++++++++++++++++++++++++++++400
  ++++++++++++++++++++++++++++++++++++++++++++++++++450
  ++++++++++++++++++++++++++++++++++++++++++++++++++500
  ++++++++++++
  END:: DRAM Persist test

  23:49:08.144 (loader): elf file is for Teensy 4.0 (IMXRT1062)
  23:49:08.149 (loader): using hex file - Teensy not configured for encryption
  DRAM Persist test
  PSRAM size 0
  ._._._._._._._._._._._._._ Bytes 510976 is 499 KB

  Start Address 0x20203088 End at 0x2027fc88
  XXXXXXXXXXXXX13+1-------------------19+++++++++++++++++50
  ++++++++++++++++++++++++++++++++++++++++++++++++++100
  ++++++++++++++++++++++++++++++++++++++++++++++++++150
  ++++++++++++++++++++++++++++++++++++++++++++++++++200
  ++++++++++++++++++++++++++++++++++++++++++++++++++250
  ++++++++++++++++++++++++++++++++++++++++++++++++++300
  ++++++++++++++++++++++++++++++++++++++++++++++++++350
  ++++++++++++++++++++++++++++++++++++++++++++++++++400
  ++++++++++++++++++++++++++++++++++++++++++++++++++450
  ++++++++++++++++++++++++++++++++++++++++++++++++++500
  ++++++++++++
  END:: DRAM Persist test
*/

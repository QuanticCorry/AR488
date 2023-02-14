#include <Arduino.h>
#include "AR488_ComPorts.h"

/***** AR488_ComPorts.cpp, ver. 0.51.09, 20/06/2022 *****/
/*
 * Serial Port implementation
 */


/*****************************/ 
/*****  DATA SERIAL PORT *****/
/*****************************/

#ifdef DATAPORT_ENABLE
  #ifdef AR_SERIAL_SWPORT

    SoftwareSerial dataPort(SW_SERIAL_RX_PIN, SW_SERIAL_TX_PIN);

    void startDataPort() {
      dataPort.begin(AR_SERIAL_SPEED);
    }

  #else

    Stream& dataPort = AR_SERIAL_PORT;

    void startDataPort() {
      AR_SERIAL_PORT.begin(AR_SERIAL_SPEED);
    }
  
  #endif

#else

  DEVNULL _dndata;
  Stream& dataPort = _dndata;

#endif  // DATAPORT_ENABLE



/*****************************/ 
/***** DEBUG SERIAL PORT *****/
/*****************************/

#ifdef DEBUG_ENABLE
  #ifdef DB_SERIAL_SWPORT

    SoftwareSerial debugPort(SW_SERIAL_RX_PIN, SW_SERIAL_TX_PIN);

    void startDebugPort() {
      debugPort.begin(DB_SERIAL_SPEED);
    }
  
  #else

    Stream& debugPort = DB_SERIAL_PORT;

    void startDebugPort() {
      DB_SERIAL_PORT.begin(DB_SERIAL_SPEED);
    }

  #endif

  void printHex(uint8_t byteval) {
    char x[4] = {'\0'};
    sprintf(x,"%02X ", byteval);
    debugPort.print(x);
  }

  void printHexBuf(char * buf, size_t bsize){
    for (size_t i = 0; i < bsize; i++) {
      printHex(buf[i]);
    }
    debugPort.println();
  }
  
  void printHexArray(uint8_t barray[], size_t asize){
    for (size_t i = 0; i < asize; i++) {
      printHex(barray[i]);
    }
    debugPort.println();
  }

#else

  DEVNULL _dndebug;
  Stream& debugPort = _dndebug;

#endif  // DEBUG_ENABLE


#ifndef AR488_CONFIG_H
#define AR488_CONFIG_H

/*********************************************/
/***** AR488 GLOBAL CONFIGURATION HEADER *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv *****/


/***** Firmware version *****/
#define FWVER "AR488 GPIB controller, ver. 0.51.15, 12/10/2022"



/***** BOARD CONFIGURATION *****/
/*
 * Platform will be selected automatically based on
 * Arduino definition.
 * Only ONE board/layout should be selected per platform
 * Only ONE Serial port can be used to receive output
 */


/*** Custom layout ***/
/*
 * Uncomment to use custom board layout
 */
//#define AR488_CUSTOM

/*
 * Configure the appropriate board/layout section
 * below as required
 */
#ifdef AR488_CUSTOM
  /* Board layout */
  /*
   * Define board layout in the AR488 CUSTOM LAYOUT
   * section below
   */
  /* Default serial port type */
  #define AR_SERIAL_TYPE_HW

/*** UNO and NANO boards ***/
#elif __AVR_ATmega328P__
  /* Board/layout selection */
  #define AR488_UNO
  //#define AR488_NANO
  //#define AR488_MCP23S17
  //#define AR488_MCP23017

/*** MEGA 32U4 based boards (Micro, Leonardo) ***/
#elif __AVR_ATmega32U4__
  /*** Board/layout selection ***/
  //#define AR488_MEGA32U4_MICRO  // Artag's design for Micro board
  #define AR488_MEGA32U4_LR3  // Leonardo R3 (same pin layout as Uno)
  
/*** MEGA 2560 board ***/
#elif __AVR_ATmega2560__
  /*** Board/layout selection ***/
  //#define AR488_MEGA2560_D
  #define AR488_MEGA2560_E1
  //#define AR488_MEGA2560_E2
  //#define AR488_MCP23S17
  //#define AR488_MCP23017

/***** Panduino Mega 644 or Mega 1284 board *****/
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
  /* Board/layout selection */
  #define AR488_MEGA644P_MCGRAW
  
#endif  // Board/layout selection



/***** SERIAL PORT CONFIGURATION *****/
/*
 * Note: On most boards the primary serial device is named Serial. On boards that have a secondary
 *       UART port this is named Serial1. The Mega2560 also supports Serial2, Serial3 and Serial4.
 *       When using layout AR488_MEGA2560_D, Serial2 pins are assigned as GPIO pins for the GPIB bus
 *       so Serial2 is not available.
 */
/***** Communication port *****/
#define DATAPORT_ENABLE
#ifdef DATAPORT_ENABLE
  // Serial port device
  #define AR_SERIAL_PORT Serial
  // #define AR_SERIAL_SWPORT
  // Set port operating speed
  #define AR_SERIAL_SPEED 115200
  // Enable Bluetooth (HC05) module?
  //#define AR_SERIAL_BT_ENABLE 12        // HC05 enable pin
  //#define AR_SERIAL_BT_NAME "AR488-BT"  // Bluetooth device name
  //#define AR_SERIAL_BT_CODE "488488"    // Bluetooth pairing code
#endif

/***** Debug port *****/
//#define DEBUG_ENABLE
#ifdef DEBUG_ENABLE
  // Serial port device
  #define DB_SERIAL_PORT Serial3
  // #define DB_SERIAL_SWPORT
  // Set port operating speed
  #define DB_SERIAL_SPEED 115200
#endif

/***** Configure SoftwareSerial Port *****/
/*
 * Configure the SoftwareSerial TX/RX pins and baud rate here
 * Note: SoftwareSerial support conflicts with PCINT support
 * When using SoftwareSerial, disable USE_INTERRUPTS.
 */
#if defined(AR_SERIAL_SWPORT) || defined(DB_SERIAL_SWPORT)
  #define SW_SERIAL_RX_PIN 11
  #define SW_SERIAL_TX_PIN 12
#endif
/*
 * Note: SoftwareSerial reliable only up to a MAX of 57600 baud only
 */



/***** SUPPORT FOR PERIPHERAL CHIPS *****/
/*
 * This sections priovides configuration to enable/disable support
 * for SN7516x, MCP23S17 and MCP23017 chips.
 */

/***** Enable SN7516x chips *****/
/*
 * Uncomment to enable the use of SN7516x GPIB tranceiver ICs.
 * This will require the use of an additional GPIO pin to control
 * the read and write modes of the ICs.
 */
#define SN7516X
#ifdef SN7516X
  //#define SN7516X_TE 51
  //#define SN7516X_DC 53
  //#define SN7516X_SC 12
  // ONLYA board
  #define SN7516X_TE 43
  #define SN7516X_DC 45
#endif




/***** MISCELLANEOUS OPTIONS *****/
/*
 * Miscellaneous options
 */


/***** Pin State Detection *****/
/*
 * With UNO. NANO and MEGA boards with pre-defined layouts,
 * USE_INTERRUPTS can and should be used.
 * With the AR488_CUSTOM layout and unknown boards, USE_INTERRUPTS must  
 * be commented out. Interrupts are used on pre-defined AVR board layouts 
 * and will respond faster, however in-loop checking for state of pin states
 * can be supported with any board layout.
 */
 
#ifdef __AVR__
  // For supported boards use interrupt handlers
  #if defined (AR488_UNO) || defined (AR488_NANO) || defined (AR488_MEGA2560) || defined (AR488_MEGA32U4)
    #ifndef AR488_CUSTOM
      #ifndef AR488_MCP23S17
        #define USE_INTERRUPTS
      #endif
    #endif
  #endif
#endif



/***** Local/remote signal (LED) *****/
//#define REMOTE_SIGNAL_PIN 7


/***** 8-way address DIP switch *****/
//#define DIP_SWITCH
#ifdef DIP_SWITCH
#define DIP_SW_1  A8
#define DIP_SW_2  A9
#define DIP_SW_3  A10
#define DIP_SW_4  A11
#define DIP_SW_5  A12
#define DIP_SW_6  A13
#define DIP_SW_7  A14
#define DIP_SW_8  A15

#endif

#define VIN_24  A0
#define VIN_12  A1
#define V_TEMP  A2

#define SS_ETHERNET   10
#define GP_LED        7
#define FAN_ON        19
#define STATUS_IND    20
#define POWERGOOD     21
#define SR1_IN        23
#define SPICLK1       31
#define L_CLK1        29
#define CLRb          27
#define SR1_OUT       25
#define SR2_IN        41
#define SPICLK2       39
#define L_CLK2        37
#define PLb           35
#define SR2_OUT       33

/***** DEBUG LEVEL OPTIONS *****/
/*
 * Configure debug level options
 */

#ifdef DEBUG_ENABLE
  // Main module
  #define DEBUG_SERIAL_INPUT    // serialIn_h(), parseInput_h()
  //#define DEBUG_CMD_PARSER      // getCmd()
  //#define DEBUG_SEND_TO_INSTR   // sendToInstrument();
  //#define DEBUG_SPOLL           // spoll_h()
  //#define DEBUG_DEVICE_ATN      // attnRequired()
  //#define DEBUG_IDFUNC          // ID command

  // AR488_GPIBbus module
  #define DEBUG_GPIBbus_RECEIVE // GPIBbus::receiveData(), GPIBbus::readByte()
  //#define DEBUG_GPIBbus_SEND    // GPIBbus::sendData()
  //#define DEBUG_GPIBbus_CONTROL // GPIBbus::setControls() 
  //#define DEBUG_GPIB_COMMANDS   // GPIBbus::sendCDC(), GPIBbus::sendLLO(), GPIBbus::sendLOC(), GPIBbus::sendGTL(), GPIBbus::sendMSA() 
  //#define DEBUG_GPIB_ADDRESSING // GPIBbus::sendMA(), GPIBbus::sendMLA(), GPIBbus::sendUNT(), GPIBbus::sendUNL() 
  //#define DEBUG_GPIB_DEVICE     // GPIBbus::unAddressDevice(), GPIBbus::addressDevice
  
  // GPIB layout module
  //#define DEBUG_LAYOUTS

  // EEPROM module
  //#define DEBUG_EEPROM          // EEPROM

  // AR488 Bluetooth module
  //#define DEBUG_BLUETOOTH       // bluetooth
#endif


/***** ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ *****/
/***** AR488 GLOBAL CONFIGURATION HEADER *****/
/*********************************************/


/*******************************/
/***** AR488 CUSTOM LAYOUT *****/
/***** vvvvvvvvvvvvvvvvvvv *****/
#ifdef AR488_CUSTOM

#define DIO1  A0  /* GPIB 1  */
#define DIO2  A1  /* GPIB 2  */
#define DIO3  A2  /* GPIB 3  */
#define DIO4  A3  /* GPIB 4  */
#define DIO5  A4  /* GPIB 13 */
#define DIO6  A5  /* GPIB 14 */
#define DIO7  4   /* GPIB 15 */
#define DIO8  5   /* GPIB 16 */

#define IFC   8   /* GPIB 9  */
#define NDAC  9   /* GPIB 8  */
#define NRFD  10  /* GPIB 7  */
#define DAV   11  /* GPIB 6  */
#define EOI   12  /* GPIB 5  */

#define SRQ   2   /* GPIB 10 */
#define REN   3   /* GPIB 17 */
#define ATN   7   /* GPIB 11 */

#endif
/***** ^^^^^^^^^^^^^^^^^^^ *****/
/***** AR488 CUSTOM LAYOUT *****/
/*******************************/



/********************************/
/***** AR488 MACROS SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvv *****/

/*
 * (See the AR488 user manual for details)
 */

/***** Enable Macros *****/
/*
 * Uncomment to enable macro support. The Startup macro allows the
 * interface to be configured at startup. Macros 1 - 9 can be
 * used to execute a sequence of commands with a single command
 * i.e, ++macro n, where n is the number of the macro
 * 
 * USE_MACROS must be enabled to enable the macro feature including 
 * MACRO_0 (the startup macro). RUN_STARTUP must be uncommented to 
 * run the startup macro when the interface boots up
 */
//#define USE_MACROS    // Enable the macro feature
//#define RUN_STARTUP   // Run MACRO_0 (the startup macro)

#ifdef USE_MACROS

/***** Startup Macro *****/

#define MACRO_0 "\
++addr 9\n\
++auto 2\n\
*RST\n\
:func 'volt:ac'\
"
/* End of MACRO_0 (Startup macro)*/

/***** User macros 1-9 *****/

#define MACRO_1 "\
++addr 3\n\
++auto 0\n\
M3\n\
"
/*<-End of macro*/

#define MACRO_2 "\
"
/*<-End of macro 2*/

#define MACRO_3 "\
"
/*<-End of macro 3*/

#define MACRO_4 "\
"
/*<-End of macro 4*/

#define MACRO_5 "\
"
/*<-End of macro 5*/

#define MACRO_6 "\
"
/*<-End of macro 6*/

#define MACRO_7 "\
"
/*<-End of macro 7*/

#define MACRO_8 "\
"
/*<-End of macro 8*/

#define MACRO_9 "\
"
/*<-End of macro 9*/


#endif
/***** ^^^^^^^^^^^^^^^^^^^^ *****/
/***** AR488 MACROS SECTION *****/
/********************************/


/******************************************/
/***** !!! DO NOT EDIT BELOW HERE !!! *****/
/******vvvvvvvvvvvvvvvvvvvvvvvvvvvvvv******/



/*********************************************/
/***** MISCELLANEOUS DECLARATIONS *****/
/******vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv******/

#define AR_CFG_SIZE 84

typedef union shiftRegister
{
  struct pin
  {
    bool on1_1 : 1;
    bool on1_2 : 1;
    bool on1_3 : 1;
    bool on1_4 : 1;
    bool on1_5 : 1;
    bool on1_6 : 1;
    bool on2_1 : 1;
    bool on2_2 : 1;
    bool on2_3 : 1;
    bool on2_4 : 1;
    bool on2_5 : 1;
    bool on2_6 : 1;
    bool on3_1 : 1;
    bool on3_2 : 1;
    bool on3_3 : 1;
    bool on3_4 : 1;
    bool on3_5 : 1;
    bool on3_6 : 1;
    bool on4_1 : 1;
    bool on4_2 : 1;
    bool on4_3 : 1;
    bool on4_4 : 1;
    bool on4_5 : 1;
    bool on4_6 : 1;
    bool on5_1 : 1;
    bool on5_2 : 1;
    bool on5_3 : 1;
    bool on5_4 : 1;
    bool on5_5 : 1;
    bool on5_6 : 1;
    bool on6_1 : 1;
    bool on6_2 : 1;
    bool on6_3 : 1;
    bool on6_4 : 1;
    bool on6_5 : 1;
    bool on6_6 : 1;
    bool on7_1 : 1;
    bool on7_2 : 1;
    bool on7_3 : 1;
    bool on7_4 : 1;
    bool on7_5 : 1;
    bool on7_6 : 1;
    bool on8_1 : 1;
    bool on8_2 : 1;
    bool on8_3 : 1;
    bool on8_4 : 1;
    bool on8_5 : 1;
    bool on8_6 : 1;
    bool on9_1 : 1;
    bool on9_2 : 1;
    bool on9_3 : 1;
    bool on9_4 : 1;
    bool on9_5 : 1;
    bool on9_6 : 1;
    bool on10_1 : 1;
    bool on10_2 : 1;
    bool on10_3 : 1;
    bool on10_4 : 1;
    bool on10_5 : 1;
    bool on10_6 : 1;
    bool on11_1 : 1;
    bool on11_2 : 1;
    bool on11_3 : 1;
    bool on11_4 : 1;
    bool on11_5 : 1;
    bool on11_6 : 1;
    bool on12_1 : 1;
    bool on12_2 : 1;
    bool on12_3 : 1;
    bool on12_4 : 1;
    bool on12_5 : 1;
    bool on12_6 : 1;
    bool on13_1 : 1;
    bool on13_2 : 1;
    bool on13_3 : 1;
    bool on13_4 : 1;
    bool on13_5 : 1;
    bool on13_6 : 1;
    bool on14_1 : 1;
    bool on14_2 : 1;
    bool on14_3 : 1;
    bool on14_4 : 1;
    bool on14_5 : 1;
    bool on14_6 : 1;
    bool test1 : 1;
    bool test2 : 1;
    bool test3 : 1;
    bool test4 : 1;
    bool on15_1 : 1;
    bool on15_2 : 1;
    bool on16_1 : 1;
    bool on16_2 : 1;
    bool on17_1 : 1;
    bool on17_2 : 1;
    bool on18_1 : 1;
    bool on18_2 : 1;
    bool on19_1 : 1;
    bool on19_2 : 1;
    bool on20_1 : 1;
    bool on20_2 : 1;
    bool on21_1 : 1;
    bool on21_2 : 1;
    bool on22_1 : 1;
    bool on22_2 : 1;
    bool on23_1 : 1;
    bool on23_2 : 1;
    bool on24_1 : 1;
    bool on24_2 : 1;
    bool on25_1 : 1;
    bool on25_2 : 1;
    bool on26_1 : 1;
    bool on26_2 : 1;
    bool on27_1 : 1;
    bool on27_2 : 1;
    bool on28_1 : 1;
    bool on28_2 : 1;
    bool on29_1 : 1;
    bool on29_2 : 1;
    bool on30_1 : 1;
    bool on30_2 : 1;
    bool on31_1 : 1;
    bool on31_2 : 1;
    bool on32_1 : 1;
    bool on32_2 : 1;
    bool on33_1 : 1;
    bool on33_2 : 1;
    bool on34_1 : 1;
    bool on34_2 : 1;
    bool on35_1 : 1;
    bool on35_2 : 1;
    bool on36_1 : 1;
    bool on36_2 : 1;
    bool on37_1 : 1;
    bool on37_2 : 1;
    bool on38_1 : 1;
    bool on38_2 : 1;
    bool on39_1 : 1;
    bool on39_2 : 1;
    bool on40_1 : 1;
    bool on40_2 : 1;
    bool on41_1 : 1;
    bool on41_2 : 1;
    bool on42_1 : 1;
    bool on42_2 : 1;
    bool on43_1 : 1;
    bool on43_2 : 1;
    bool on44_1 : 1;
    bool on44_2 : 1;
    bool on45_1 : 1;
    bool on45_2 : 1;
    bool on46_1 : 1;
    bool on46_2 : 1;
    bool on47_1 : 1;
    bool on47_2 : 1;
    bool on48_1 : 1;
    bool on48_2 : 1;
    bool on49_1 : 1;
    bool on49_2 : 1;
    bool on50_1 : 1;
    bool on50_2 : 1;      
  } pin;
  uint8_t byte[20];
} shiftRegister;

/******^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^******/
/***** MISCELLANEOUS DECLARATIONS *****/
/*********************************************/





#endif // AR488_CONFIG_H

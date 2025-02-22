
#ifdef __AVR__
#include <avr/wdt.h>
#endif

#include "AR488_Config.h"
#include "AR488_GPIBbus.h"
#include "AR488_ComPorts.h"
#include "AR488_Eeprom.h"
#include <SPI.h>
#include <Ethernet.h>

//TODO
/*
Status LED
Power LED and analog input to read 12V nd 24V inputs
ability to change IP address???
*/



/*******************************/
/***** SERIAL PARSE BUFFER *****/
/***** vvvvvvvvvvvvvvvvvvv *****/
/*
 * Note: Arduino serial input buffer is 64 
 */
/***** ^^^^^^^^^^^^^^^^^^^ *****/
/***** SERIAL INPUT PARSE BUFFER *****/
/*******************************/
static const uint8_t PBSIZE = 128;
char pBuf[PBSIZE];
uint8_t pbPtr = 0;

/*******************************/
/***** GPIB PARSE BUFFER *****/
/***** vvvvvvvvvvvvvvvvvvv *****/

static const uint8_t GPIB_BSIZE = 64;
uint8_t GPIB_Buf[GPIB_BSIZE];
uint8_t gpib_Ptr = 0;

/***** ^^^^^^^^^^^^^^^^^^^ *****/
/***** SHIFT REGISTER BUFFER *****/
/*******************************/

static union shiftRegister SRbuf;
static union shiftRegister SRindBuf;

//start GPIB section

/*
   Implements most of the CONTROLLER functions;
   Substantially compatible with 'standard' Prologix "++" commands
   (see +savecfg command in the manual for differences)

   Principle of operation:
   - Commands received from USB are buffered and whole terminated lines processed
   - Interface commands prefixed with "++" are passed to the command handler
   - Instrument commands and data not prefixed with '++' are sent directly to the GPIB bus.
   - To receive from the instrument, issue a ++read command or put the controller in auto mode (++auto 1|2)
   - Characters received over the GPIB bus are unbuffered and sent directly to USB
   NOTES:
   - GPIB line in a HIGH state is un-asserted
   - GPIB line in a LOW state is asserted
   - The ATMega processor control pins have a high impedance when set as inputs
   - When set to INPUT_PULLUP, a 10k pull-up (to VCC) resistor is applied to the input
*/

/*
   For information regarding the GPIB firmware by Emanualle Girlando see:
   http://egirland.blogspot.com/2014/03/arduino-uno-as-usb-to-gpib-controller.html
*/


/*
   Pin mapping between the Arduino pins and the GPIB connector.
   NOTE:
   GPIB pins 10 and 18-24 are connected to GND
   GPIB pin 12 should be connected to the cable shield (might be n/c)
   Pin mapping follows the layout originally used by Emanuelle Girlando, but adds
   the SRQ line (GPIB 10) on pin 2 and the REN line (GPIB 17) on pin 13. The program
   should therefore be compatible with the original interface design but for full
   functionality will need the remaining two pins to be connected.
   For further information about the AR488 see the AR488 Manual. 
*/

/*************************************/
/***** MACRO STRUCTRURES SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvvv *****/
#ifdef USE_MACROS

/*** DO NOT MODIFY ***/
/*** vvvvvvvvvvvvv ***/

/***** STARTUP MACRO *****/
const char startup_macro[] PROGMEM = { MACRO_0 };

/***** Consts holding USER MACROS 1 - 9 *****/
const char macro_1[] PROGMEM = { MACRO_1 };
const char macro_2[] PROGMEM = { MACRO_2 };
const char macro_3[] PROGMEM = { MACRO_3 };
const char macro_4[] PROGMEM = { MACRO_4 };
const char macro_5[] PROGMEM = { MACRO_5 };
const char macro_6[] PROGMEM = { MACRO_6 };
const char macro_7[] PROGMEM = { MACRO_7 };
const char macro_8[] PROGMEM = { MACRO_8 };
const char macro_9[] PROGMEM = { MACRO_9 };


/* Macro pointer array */
const char *const macros[] PROGMEM = {
  startup_macro,
  macro_1,
  macro_2,
  macro_3,
  macro_4,
  macro_5,
  macro_6,
  macro_7,
  macro_8,
  macro_9
};

/*** ^^^^^^^^^^^^^ ***/
/*** DO NOT MODIFY ***/

#endif

/**************************/
/***** HELP MESASAGES *****/
/****** vvvvvvvvvvvvv *****/

static const char cmdHelp[] PROGMEM = {
  "addr:P Display/set device address\n"
  "auto:P Automatically request talk and read response\n"
  "clr:P Send Selected Device Clear to current GPIB address\n"
  "eoi:P Enable/disable assertion of EOI signal\n"
  "eor:P Show or set end of receive character(s)\n"
  "eos:P Specify GPIB termination character\n"
  "eot_char:P Set character to append to USB output when EOT enabled\n"
  "eot_enable:P Enable/Disable appending user specified character to USB output on EOI detection\n"
  "help:P This message\n"
  "ifc:P Assert IFC signal for 150 miscoseconds - make AR488 controller in charge\n"
  "llo:P Local lockout - disable front panel operation on instrument\n"
  "loc:P Enable front panel operation on instrument\n"
  "lon:P Put controller in listen-only mode (listen to all traffic)\n"
  "mode:P Set the interface mode (0=controller/1=device)\n"
  "read:P Read data from instrument\n"
  "read_tmo_ms:P Read timeout specified between 1 - 3000 milliseconds\n"
  "rst:P Reset the controller\n"
  "savecfg:P Save configration\n"
  "spoll:P Serial poll the addressed host or all instruments\n"
  "srq:P Return status of srq signal (1-srq asserted/0-srq not asserted)\n"
  "status:P Set the status byte to be returned on being polled (bit 6 = RQS, i.e SRQ asserted)\n"
  "trg:P Send trigger to selected devices (up to 15 addresses)\n"
  "ver:P Display firmware version\n"
  "aspoll:C Serial poll all instruments (alias: ++spoll all)\n"
  "dcl:C Send unaddressed (all) device clear  [power on reset] (is the rst?)\n"
  "default:C Set configuration to controller default settings\n"
  "id:C Show interface ID information - see also: 'id name'; 'id serial'; 'id verstr'\n"
  "id name:C Show/Set the name of the interface\n"
  "id serial:C Show/Set the serial number of the interface\n"
  "id verstr:C Show/Set the version string sent in reply to ++ver e.g. \"GPIB-USB\"). Max 47 chars, excess truncated.\n"
  "idn:C Enable/Disable reply to *idn? (disabled by default)\n"
  "macro:C Run a macro (if macro support is compiled)\n"
  "ppoll:C Conduct a parallel poll\n"
  "ren:C Assert or Unassert the REN signal\n"
  "repeat:C Repeat a given command and return result\n"
  "setvstr:C DEPRECATED - see id verstr\n"
  "srqauto:C Automatically condiuct serial poll when SRQ is asserted\n"
  "ton:C Put controller in talk-only mode (send data only)\n"
  "verbose:C Verbose (human readable) mode\n"
  "xdiag:C Bus diagnostics (see the doc)\n"
};

/***** ^^^^^^^^^^^^^ *****/
/***** HELP MESSAGES *****/
/*************************/

/************************************/
/***** COMMON VARIABLES SECTION *****/
/***** vvvvvvvvvvvvvvvvvvvvvvvv *****/

/****** Process status values *****/
#define OK 0
#define ERR 1

/***** Control characters *****/
#define ESC 0x1B   // the USB escape char
#define CR 0xD     // Carriage return
#define LF 0xA     // Newline/linefeed
#define PLUS 0x2B  // '+' character

/****** Global variables with volatile values related to controller state *****/

// GPIB bus object
GPIBbus gpibBus;

// GPIB control state
//uint8_t cstate = 0;

// Verbose mode
bool isVerb = false;

// CR/LF terminated line ready to process
uint8_t lnRdy = 0;
// only do first time through main loop
bool isInit = true;

// GPIB data receive flags
bool autoRead = false;         // Auto reading (auto mode 3) GPIB data in progress
bool readWithEoi = false;      // Read eoi requested
bool readWithEndByte = false;  // Read with specified terminator character
bool isQuery = false;          // Direct instrument command is a query
uint8_t tranBrk = 0;           // Transmission break on 1=++, 2=EOI, 3=ATN 4=UNL
uint8_t endByte = 0;           // Termination character

// Escaped character flag
bool isEsc = false;          // Charcter escaped
bool isPlusEscaped = false;  // Plus escaped

// Read only mode flag
bool isRO = false;

// Talk only mode flag
uint8_t isTO = 0;

bool isProm = false;

// Data send mode flags
bool dataBufferFull = false;  // Flag when parse buffer is full

// SRQ auto mode
bool isSrqa = false;

// Whether to run Macro 0 (macros must be enabled)
uint8_t runMacro = 0;

// Send response to *idn?
bool sendIdn = false;

//number of bytes read from GPIB
uint8_t GPIB_length = 0;
//end GPIB section

// start Ethernet section
byte mac[] = {

  0xA8, 0x61, 0x0A, 0xAE, 0x97, 0x46
};

IPAddress ip(192, 168, 1, 177);

const int numberOfClients = 2;
const int sizeOfBuffer = 80;

// telnet defaults to port 23

EthernetServer server(23);

EthernetClient clients[numberOfClients];

bool linkOn = false;

// end Ethernet section

/*******************************/
/***** COMMON CODE SECTION *****/
/***** vvvvvvvvvvvvvvvvvvv *****/

bool powerGood = false;
bool fanOn = false;
bool statusLedState = false;
const int statusLedInterval = 1000;
unsigned long lastTime = millis();
unsigned long lastTimeMicro = micros();
bool gpLed = false;
uint16_t analog24V = 0;
uint16_t analog12V = 0;
uint16_t analogTemp = 0;


/******  Arduino standard SETUP procedure *****/
void setup() {

  // Disable the watchdog (needed to prevent WDT reset loop)
#ifdef __AVR__
  wdt_disable();
#endif

//pinMode(45, OUTPUT); //DC pin on SN75161
//digitalWrite(45, LOW); //DC pin on SN75161
pinMode(GP_LED, OUTPUT);
digitalWrite(GP_LED, LOW);
pinMode(FAN_ON, OUTPUT);
digitalWrite(FAN_ON, LOW);
pinMode(STATUS_IND, OUTPUT);
digitalWrite(STATUS_IND, HIGH);
pinMode(POWERGOOD, OUTPUT);
digitalWrite(POWERGOOD, HIGH);
pinMode(SR1_IN, OUTPUT);
digitalWrite(SR1_IN, LOW);
pinMode(SPICLK1, OUTPUT);
digitalWrite(SPICLK1, LOW);
pinMode(L_CLK1, OUTPUT);
digitalWrite(L_CLK1, LOW);
pinMode(CLRb, OUTPUT);
digitalWrite(CLRb, LOW);
pinMode(SR2_IN, OUTPUT);
digitalWrite(SR2_IN, LOW);
pinMode(SPICLK2, OUTPUT);
digitalWrite(SPICLK2, LOW);
pinMode(L_CLK2, OUTPUT);
digitalWrite(L_CLK2, LOW);
pinMode(PLb, OUTPUT);
digitalWrite(PLb, HIGH);
pinMode(SR1_OUT, INPUT);
pinMode(SR2_OUT, INPUT);
pinMode(DIP_SW_1, INPUT);
pinMode(DIP_SW_2, INPUT);
pinMode(DIP_SW_3, INPUT);
pinMode(DIP_SW_4, INPUT);
pinMode(DIP_SW_5, INPUT);
pinMode(DIP_SW_6, INPUT);
pinMode(DIP_SW_7, INPUT);
pinMode(DIP_SW_8, INPUT);

byte DipAddress = 0;
DipAddress |= (digitalRead(DIP_SW_8) << 7);
DipAddress |= (digitalRead(DIP_SW_7) << 6);
DipAddress |= (digitalRead(DIP_SW_6) << 5);
DipAddress |= (digitalRead(DIP_SW_5) << 4);
DipAddress |= (digitalRead(DIP_SW_4) << 3);
DipAddress |= (digitalRead(DIP_SW_3) << 2);
DipAddress |= (digitalRead(DIP_SW_2) << 1);
DipAddress |= digitalRead(DIP_SW_1);

IPAddress ip(192, 168, 1, DipAddress);

//start Ethernet setup
  Ethernet.init(SS_ETHERNET);  // Most Arduino shields
  // initialize the Ethernet device
  Ethernet.begin(mac, ip);
    // Initialise serial at the configured baud rate
  AR_SERIAL_PORT.begin(AR_SERIAL_SPEED);
  while (!AR_SERIAL_PORT) {
//TODO remove loop
    ; // wait for serial port to connect. Needed for native USB port only

  }

  // Turn off internal LED (set OUPTUT/LOW)
#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    AR_SERIAL_PORT.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
  }

  // start listening for clients

  server.begin();

  AR_SERIAL_PORT.print("Server address:");

  AR_SERIAL_PORT.println(Ethernet.localIP());
//end Ethernet setup

//start GPIB setup
  #ifdef PIN_REMOTE
  pinMode(PIN_REMOTE, OUTPUT);
  digitalWrite(PIN_REMOTE, LOW);
#endif

  // Initialise parse buffer
  flushPbuf();

#ifdef DEBUG_ENABLE
  // Initialise debug port
  DB_SERIAL_PORT.begin(DB_SERIAL_SPEED);
#endif

#ifdef E2END
  //  DB_RAW_PRINTLN(F("EEPROM detected!"));
  // Read data from non-volatile memory
  //(will only read if previous config has already been saved)
  if (!isEepromClear()) {
    //DB_RAW_PRINTLN(F("EEPROM has data."));
    if (!epReadData(gpibBus.cfg.db, GPIB_CFG_SIZE)) {
      // CRC check failed - config data does not match EEPROM
      //DB_RAW_PRINTLN(F("CRC check failed. Erasing EEPROM...."));
      epErase();
      gpibBus.setDefaultCfg();
      //      initAR488();
      epWriteData(gpibBus.cfg.db, GPIB_CFG_SIZE);
      //DB_RAW_PRINTLN(F("EEPROM data set to default."));
    }
  }
#endif

//Set GPIB adddress based on DIP switches
if(DipAddress < 30)
{
  gpibBus.cfg.paddr = DipAddress;
} else
{
  gpibBus.cfg.paddr = 30;  
}


  // SN7516x IC support
#ifdef SN7516X
  pinMode(SN7516X_TE, OUTPUT);
#ifdef SN7516X_DC
  pinMode(SN7516X_DC, OUTPUT);
#endif
  if (gpibBus.cfg.cmode == 2) {
    // Set controller mode on SN75161/2
    digitalWrite(SN7516X_TE, LOW);
#ifdef SN7516X_DC
    digitalWrite(SN7516X_DC, LOW);
#endif
#ifdef SN7516X_SC
    digitalWrite(SN7516X_SC, HIGH);
#endif
  } else {
    // Set listen mode on SN75161/2 (default)
    digitalWrite(SN7516X_TE, HIGH);
#ifdef SN7516X_DC
    digitalWrite(SN7516X_DC, HIGH);
#endif
#ifdef SN7516X_SC
    digitalWrite(SN7516X_SC, LOW);
#endif
  }
#endif

  // Start the interface in the configured mode
  delay(2000);
  if(Ethernet.linkStatus() == LinkOFF)
  {
    gpibBus.begin();
    linkOn = false;    
  }else
  {
    linkOn = true;
  }

digitalWrite(CLRb, HIGH);
#if defined(USE_MACROS) && defined(RUN_STARTUP)
  // Run startup macro
  execMacro(0);
#endif

  dataPort.flush();
//end GPIB setup


}
/****** End of Arduino standard SETUP procedure *****/


/***** ARDUINO MAIN LOOP *****/
void loop() {

  bool errFlg = false;

  if(millis() >= (statusLedInterval + lastTime))
  {
    lastTime = millis();
    statusLedState = !statusLedState;
    digitalWrite(STATUS_IND, statusLedState);

    analog24V = analogRead(A0);
    analog12V = analogRead(A1);
    analogTemp = analogRead(A2);

    //Handle input voltage and temperature checks for fan turn on
    if((analog24V > 752) && (analog24V < 889) && //A0 between 22V-26V
      (analog12V > 342) && (analog12V < 479))   //A1 between 10V-14V
    {
      digitalWrite(POWERGOOD, HIGH);
    } 
    else
    {
      digitalWrite(POWERGOOD, LOW);
    }  
    // check if we should change fan state
    if((!fanOn) && (analogTemp > 193)) // 28 degrees Celcius
    {
      fanOn = true;
      digitalWrite(FAN_ON, HIGH);
    }
    // 3 degrees Celsius of hysteresis
    if((fanOn) && (analogTemp <= 182)) // 25 degrees Celcius
    {
      fanOn = false;
      digitalWrite(FAN_ON, LOW);
    }
  }
  else
  {
    if(lastTime > millis()) lastTime = millis(); //when millis() rolls over after about 50 days
  }

  // check for any new client connecting
  if (linkOn)
  {

    EthernetClient newClient = server.accept();

    if (newClient) {

      for (byte i=0; i < numberOfClients; i++) {

        if (!clients[i]) {

          AR_SERIAL_PORT.print("We have a new client #");

          AR_SERIAL_PORT.println(i);

          // Once we "accept", the client is no longer tracked by EthernetServer
          // so we must store it into our list of clients

          clients[i] = newClient;

          break;

        }
      }
    }

    // check for incoming data from all clients

    for (byte i=0; i < numberOfClients; i++) {

      if (clients[i] && clients[i].available() > 0) {

        // read bytes from a client
            
        byte buffer[sizeOfBuffer];

        int count = clients[i].read(buffer, sizeOfBuffer);

        if((buffer[count-1] == 0xA) && (buffer[count-2] == 0xD))
        {
          AR_SERIAL_PORT.println(count);     
          for (byte j=0; j < count; j++) {
            AR_SERIAL_PORT.print((char) buffer[j]);
          }
          clients[i].println(parse(&buffer[0]));
        }
      }
    }

    // stop any clients which disconnect

    for (byte i=0; i < numberOfClients; i++) {

      if (clients[i] && !clients[i].connected()) {

        AR_SERIAL_PORT.print("disconnect client #");

        AR_SERIAL_PORT.println(i);

        clients[i].stop();

      }
    }
  }
  else 
  {
    if(isInit)
    {
      isInit = false;
    }
  /*** Macros ***/
  /*
  * Run the startup macro if enabled
  */
  #ifdef USE_MACROS
    // Run user macro if flagged
    if (runMacro > 0) {
      execMacro(runMacro);
      runMacro = 0;
    }
  #endif


    /*** Process the buffer ***/
    /* Each received char is passed through parser until an un-escaped 
  * CR is encountered. If we have a command then parse and execute.
  * If the line is data (inclding direct instrument commands) then
  * send it to the instrument.
  * NOTE: parseInput() sets lnRdy in serialEvent, readBreak or in the
  * above loop
  * lnRdy=1: process command;
  * lnRdy=2: send data to Gpib
  */

    // lnRdy=1: received a command so execute it...
    if (lnRdy == 1) 
    {
      if (autoRead) 
      {
        // Issuing any command stops autoread mode
        autoRead = false;
        gpibBus.unAddressDevice();
      }
      execCmd(pBuf, pbPtr);
    }

    // Device mode:
    if (gpibBus.isController() == false)
    {
      if (isTO > 0) 
      {
        tonMode();
      } else if (isRO) 
      {
        lonMode();
      } else if (gpibBus.isAsserted(ATN)) 
      {
        digitalWrite(GP_LED, HIGH);
        attnRequired();
      }

      // Can't send in LON mode so just clear the buffer
      if (isProm) 
      {
        if (lnRdy == 2) flushPbuf();
      }
    }

    // If charaters waiting in the serial input buffer then call handler
    if (dataPort.available()) lnRdy = serialIn_h();

    delayMicroseconds(5);
    }
  }
  /***** END MAIN LOOP *****/

  /***** Initialise device mode *****/
  void initDevice() {
    gpibBus.stop();
    gpibBus.startDeviceMode();
  }

  /***** Initialise controller mode *****/
  void initController() {
    gpibBus.stop();
    gpibBus.startControllerMode();
  }

  /***** Serial event handler *****/
  /*
  * Note: the Arduino serial buffer is 64 characters long. Characters are stored in
  * this buffer until serialEvent_h() is called. parsedInput() takes a character at 
  * a time and places it into the 256 character parse buffer whereupon it is parsed
  * to determine whether a command or data are present.
  * lnRdy=0: terminator not detected yet
  * lnRdy=1: terminator detected, sequence in parse buffer is a ++ command
  * lnRdy=2: terminator detected, sequence in parse buffer is data or direct instrument command
  */
  uint8_t serialIn_h() {
    uint8_t bufferStatus = 0;
    // Parse serial input until we have detected a line terminator
    while (dataPort.available() && bufferStatus == 0) {  // Parse while characters available and line is not complete
      bufferStatus = parseInput(dataPort.read());
    }

  #ifdef DEBUG_SERIAL_INPUT
    if (bufferStatus) {
      DB_PRINT(F("bufferStatus: "), bufferStatus);
    }
  #endif

    return bufferStatus;
  }


  /*************************************/
  /***** Device operation routines *****/
  /*************************************/


  /***** Unrecognized command *****/
  void errBadCmd() {
    dataPort.println(F("Unrecognized command"));
  }


  /***** Add character to the buffer and parse *****/
  uint8_t parseInput(char c) {

    uint8_t r = 0;
    /*
    if (xonxoff){
      // Send XOFF when buffer around 80% full
      if (pbPtr < (PBSIZE*0.8)) dataPort.print(0x13);
    }
  */
    // Read until buffer full
    if (pbPtr < PBSIZE) {
      if (isVerb && c != LF) dataPort.print(c);  // Humans like to see what they are typing...
      // Actions on specific characters
      switch (c) {
        // Carriage return or newline? Then process the line
        case CR:
        case LF:
          // If escaped just add to buffer
          if (isEsc) {
            addPbuf(c);
            isEsc = false;
          } else {
            // Carriage return on blank line?
            // Note: for data CR and LF will always be escaped
            if (pbPtr == 0) {
              flushPbuf();
              if (isVerb) {
                dataPort.println();
                showPrompt();
              }
              return 0;
            } else {
  //            if (isVerb) dataPort.println();  // Move to new line
  #ifdef DEBUG_SERIAL_INPUT
              DB_PRINT(F("parseInput: Received "), pBuf);
  #endif
              // Buffer starts with ++ and contains at least 3 characters - command?
              if (pbPtr > 2 && isCmd(pBuf) && !isPlusEscaped) {
                // Exclamation mark (break read loop command)
                if (pBuf[2] == 0x21) {
                  r = 3;
                  flushPbuf();
                  // Otherwise flag command received and ready to process
                } else {
                  r = 1;
                }
                // Buffer contains *idn? query and interface to respond
              } else if (pbPtr > 3 && gpibBus.cfg.idn > 0 && isIdnQuery(pBuf)) {
                sendIdn = true;
                flushPbuf();
                // Buffer has at least 1 character = instrument data to send to gpib bus
              } else if (pbPtr > 0) {
                r = 2;
              }
              isPlusEscaped = false;
  #ifdef DEBUG_SERIAL_INPUT
              DB_PRINT(F("R: "), r);
  #endif
              //            return r;
            }
          }
          break;
        case ESC:
          // Handle the escape character
          if (isEsc) {
            // Add character to buffer and cancel escape
            addPbuf(c);
            isEsc = false;
          } else {
            // Set escape flag
            isEsc = true;  // Set escape flag
          }
          break;
        case PLUS:
          if (isEsc) {
            isEsc = false;
            if (pbPtr < 2) isPlusEscaped = true;
          }
          addPbuf(c);
          //        if (isVerb) dataPort.print(c);
          break;
        // Something else?
        default:  // any char other than defined above
                  //        if (isVerb) dataPort.print(c);  // Humans like to see what they are typing...
          // Buffer contains '++' (start of command). Stop sending data to serial port by halting GPIB receive.
          addPbuf(c);
          isEsc = false;
      }
    }
    if (pbPtr >= PBSIZE) {
      if (isCmd(pBuf) && !r) {  // Command without terminator and buffer full
        if (isVerb) {
          dataPort.println(F("ERROR - Command buffer overflow!"));
        }
        flushPbuf();
      } else {  // Buffer contains data and is full, so process the buffer (send data via GPIB)
        dataBufferFull = true;
        // Signal to GPIB object that more data will follow (suppress GPIB addressing)
        //      gpibBus.setDataContinuity(true);
        r = 2;
      }
    }
    return r;
  }


  /***** Is this a command? *****/
  bool isCmd(char *buffr) {
    if (buffr[0] == PLUS && buffr[1] == PLUS) {
  #ifdef DEBUG_SERIAL_INPUT
      DB_PRINT(F("Command detected."), "");
  #endif
      return true;
    }
    return false;
  }


  /***** Is this an *idn? query? *****/
  bool isIdnQuery(char *buffr) {
    // Check for upper or lower case *idn?
    if (strncasecmp(buffr, "*idn?", 5) == 0) {
  #ifdef DEBUG_SERIAL_INPUT
      DB_PRINT(F("isIdnQuery: Detected IDN query."), "");
  #endif
      return true;
    }
    return false;
  }


  /***** ++read command detected? *****/
  bool isRead(char *buffr) {
    char cmd[4];
    // Copy 2nd to 5th character
    for (int i = 2; i < 6; i++) {
      cmd[i - 2] = buffr[i];
    }
    // Compare with 'read'
    if (strncmp(cmd, "read", 4) == 0) return true;
    return false;
  }


  /***** Add character to the buffer *****/
  void addPbuf(char c) {
    pBuf[pbPtr] = c;
    pbPtr++;
  }


  /***** Clear the parse buffer *****/
  void flushPbuf() {
    memset(pBuf, '\0', PBSIZE);
    pbPtr = 0;
  }


  /***** Comand function record *****/
  struct cmdRec {
    const char *token;
    int opmode;
    void (*handler)(char *);
  };


  /***** Array containing index of accepted ++ commands *****/
  /*
  * Commands without parameters require casting to a pointer
  * requiring a char* parameter. The functon is called with
  * NULL by the command processor.
  * 
  * Format: token, mode, function_ptr
  * Mode: 1=device; 2=controller; 3=both; 
  */
  static cmdRec cmdHidx[] = {

    { "addr", 3, addr_h },
    { "allspoll", 2, (void (*)(char *))aspoll_h },
    { "auto", 2, amode_h },
    { "clr", 2, (void (*)(char *))clr_h },
    { "dcl", 2, (void (*)(char *))dcl_h },
    { "default", 3, (void (*)(char *))default_h },
    { "eoi", 3, eoi_h },
    { "eor", 3, eor_h },
    { "eos", 3, eos_h },
    { "eot_char", 3, eot_char_h },
    { "eot_enable", 3, eot_en_h },
    { "help", 3, help_h },
    { "ifc", 2, (void (*)(char *))ifc_h },
    { "id", 3, id_h },
    { "idn", 3, idn_h },
    { "llo", 2, llo_h },
    { "loc", 2, loc_h },
    { "lon", 1, lon_h },
    { "macro", 2, macro_h },
    { "mla", 2, (void (*)(char *))sendmla_h },
    { "mode", 3, cmode_h },
    { "msa", 2, sendmsa_h },
    { "mta", 2, (void (*)(char *))sendmta_h },
    { "ppoll", 2, (void (*)(char *))ppoll_h },
    { "prom", 1, prom_h },
    { "read", 2, read_h },
    { "read_tmo_ms", 2, rtmo_h },
    { "ren", 2, ren_h },
    { "repeat", 2, repeat_h },
    { "rst", 3, (void (*)(char *))rst_h },
    { "trg", 2, trg_h },
    { "savecfg", 3, (void (*)(char *))save_h },
    { "setvstr", 3, setvstr_h },
    { "spoll", 2, spoll_h },
    { "srq", 2, (void (*)(char *))srq_h },
    { "srqauto", 2, srqa_h },
    { "status", 1, stat_h },
    { "ton", 1, ton_h },
    { "unl", 2, (void (*)(char *))unlisten_h },
    { "unt", 2, (void (*)(char *))untalk_h },
    { "ver", 3, ver_h },
    { "verbose", 3, (void (*)(char *))verb_h },
    { "xdiag", 3, xdiag_h }
    //  { "xonxoff",     3, xonxoff_h   }
  };


  /***** Show a prompt *****/
  void showPrompt() {
    // Print a prompt
    dataPort.print("> ");
  }


  /****** Send data to instrument *****/
  /* Processes the parse buffer whenever a full CR or LF
  * and sends data to the instrument
  */
  void sendToInstrument(char *buffr, uint8_t dsize) {

  #ifdef DEBUG_SEND_TO_INSTR
    if (buffr[dsize] != LF) DB_RAW_PRINTLN();
    DB_HEXB_PRINT(F("Received for sending: "), buffr, dsize);
  #endif

    // Is this an instrument query command (string ending with ?)
    if (buffr[dsize - 1] == '?') isQuery = true;

    // Has controller already addressed the device? - if not then address it
    if (!gpibBus.haveAddressedDevice()) gpibBus.addressDevice(gpibBus.cfg.paddr, LISTEN);

    // Send string to instrument
    gpibBus.sendData(buffr, dsize);

    // Address device
    if (dataBufferFull) {
      dataBufferFull = false;
    } else {
      gpibBus.unAddressDevice();
    }

  #ifdef DEBUG_SEND_TO_INSTR
    DB_PRINT(F("done."), "");
  #endif

    // Show a prompt on completion?
    if (isVerb) showPrompt();

    // Flush the parse buffer
    flushPbuf();
    lnRdy = 0;
  }


  /***** Execute a command *****/
  void execCmd(char *buffr, uint8_t dsize) {
    char line[PBSIZE];

    // Copy collected chars to line buffer
    memcpy(line, buffr, dsize);

    // Flush the parse buffer
    flushPbuf();
    lnRdy = 0;

  #ifdef DEBUG_CMD_PARSER
    //  DB_PRINT(F("command received: "),"");
    DB_HEXB_PRINT(F("command received: "), line, dsize);
  #endif

    // Its a ++command so shift everything two bytes left (ignore ++) and parse
    for (int i = 0; i < dsize - 2; i++) {
      line[i] = line[i + 2];
    }
    // Replace last two bytes with a null (\0) character
    line[dsize - 2] = '\0';
    line[dsize - 1] = '\0';
  #ifdef DEBUG_CMD_PARSER
    //  DB_PRINT(F("execCmd: sent to command processor: "),"");
    DB_HEXB_PRINT(F("sent to command processor: "), line, dsize - 2);
  #endif
    // Execute the command
    if (isVerb) dataPort.println();  // Shift output to next line
    getCmd(line);

    // Show a prompt on completion?
    if (isVerb) showPrompt();
  }


  /***** Extract command and pass to handler *****/
  void getCmd(char *buffr) {

    char *token;   // Pointer to command token
    char *params;  // Pointer to parameters (remaining buffer characters)

    int casize = sizeof(cmdHidx) / sizeof(cmdHidx[0]);
    int i = 0;

  #ifdef DEBUG_CMD_PARSER
    //  debugStream.print("getCmd: ");
    //  debugStream.print(buffr); debugStream.print(F(" - length: ")); debugStream.println(strlen(buffr));
    DB_PRINT(F("command buffer: "), buffr);
    DB_PRINT(F("buffer length: "), strlen(buffr));
  #endif

    // If terminator on blank line then return immediately without processing anything
    if (buffr[0] == 0x00) return;
    if (buffr[0] == CR) return;
    if (buffr[0] == LF) return;

    // Get the first token
    token = strtok(buffr, " \t");

  #ifdef DEBUG_CMD_PARSER
    DB_PRINT(F("process token: "), token);
  #endif

    // Check whether it is a valid command token
    i = 0;
    do {
      if (strcasecmp(cmdHidx[i].token, token) == 0) break;
      i++;
    } while (i < casize);

    if (i < casize) {
      // We have found a valid command and handler
  #ifdef DEBUG_CMD_PARSER
      DB_PRINT(F("found handler for: "), cmdHidx[i].token);
  #endif
      // If command is relevant to mode then execute it
      if (cmdHidx[i].opmode & gpibBus.cfg.cmode) {
        // If its a command with parameters
        // Copy command parameters to params and call handler with parameters
        params = token + strlen(token) + 1;

        // If command parameters were specified
        if (strlen(params) > 0) {
  #ifdef DEBUG_CMD_PARSER
          DB_PRINT(F("calling handler with parameters: "), params);
  #endif
          // Call handler with parameters specified
          cmdHidx[i].handler(params);
        } else {
  #ifdef DEBUG_CMD_PARSER
          DB_PRINT(F("calling handler without parameters..."), "");
  #endif
          // Call handler without parameters
          cmdHidx[i].handler(NULL);
        }
  #ifdef DEBUG_CMD_PARSER
        DB_PRINT(F("handler done."), "");
  #endif
      } else {
        errBadCmd();
        if (isVerb) dataPort.println(F("getCmd: command not available in this mode."));
      }
    } else {
      // No valid command found
      errBadCmd();
    }
  }


  /***** Prints charaters as hex bytes *****/
  /*
  void printHex(char *buffr, int dsize) {
  #ifdef DEBUG_ENABLE
    for (int i = 0; i < dsize; i++) {
      debugStream.print(buffr[i], HEX); debugStream.print(" ");
    }
    debugStream.println();
  #endif
  }
  */

  /***** Check whether a parameter is in range *****/
  /* Convert string to integer and check whether value is within
  * lowl to higl inclusive. Also returns converted text in param
  * to a uint16_t integer in rval. Returns true if successful, 
  * false if not
  */
  bool notInRange(char *param, uint16_t lowl, uint16_t higl, uint16_t &rval) {

    // Null string passed?
    if (strlen(param) == 0) return true;

    // Convert to integer
    rval = 0;
    rval = atoi(param);

    // Check range
    if (rval < lowl || rval > higl) {
      errBadCmd();
      if (isVerb) {
        dataPort.print(F("Valid range is between "));
        dataPort.print(lowl);
        dataPort.print(F(" and "));
        dataPort.println(higl);
      }
      return true;
    }
    return false;
  }


  /***** If enabled, executes a macro *****/
  #ifdef USE_MACROS
  void execMacro(uint8_t idx) {
    char c;
    const char *macro = pgm_read_word(macros + idx);
    int ssize = strlen_P(macro);

    // Read characters from macro character array
    for (int i = 0; i < ssize; i++) {
      c = pgm_read_byte_near(macro + i);
      if (c == CR || c == LF || i == (ssize - 1)) {
        // Reached terminator or end of marcro. Add to buffer before processing
        if (i == ssize - 1) {
          // Check buffer and add character
          if (pbPtr < (PBSIZE - 1)) {
            if ((c != CR) && (c != LF)) addPbuf(c);
          } else {
            // Buffer full - clear and exit
            flushPbuf();
            return;
          }
        }
        if (isCmd(pBuf)) {
          execCmd(pBuf, strlen(pBuf));
        } else {
          sendToInstrument(pBuf, strlen(pBuf));
        }
        // Done - clear the buffer
        flushPbuf();
      } else {
        // Check buffer and add character
        if (pbPtr < (PBSIZE - 1)) {
          addPbuf(c);
        } else {
          // Exceeds buffer size - clear buffer and exit
          i = ssize;
          return;
        }
      }
    }

    // Clear the buffer ready for serial input
    flushPbuf();
  }
  #endif


  /*************************************/
  /***** STANDARD COMMAND HANDLERS *****/
  /*************************************/

  /***** Show or change device address *****/
  void addr_h(char *params) {
    //  char *param, *stat;
    //  char *param;
    uint16_t val;
    if (params != NULL) {

      // Primary address
      //    param = strtok(params, " \t");
      //    if (notInRange(param, 1, 30, val)) return;
      if (notInRange(params, 1, 30, val)) return;
      if (val == gpibBus.cfg.caddr) {
        errBadCmd();
        if (isVerb) dataPort.println(F("That is my address! Address of a remote device is required."));
        return;
      }
      gpibBus.cfg.paddr = val;
      if (isVerb) {
        dataPort.print(F("Set device primary address to: "));
        dataPort.println(val);
      }
    } else {
      dataPort.println(gpibBus.cfg.paddr);
    }
  }


  /***** Show or set read timout *****/
  void rtmo_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 1, 32000, val)) return;
      gpibBus.cfg.rtmo = val;
      if (isVerb) {
        dataPort.print(F("Set [read_tmo_ms] to: "));
        dataPort.print(val);
        dataPort.println(F(" milliseconds"));
      }
    } else {
      dataPort.println(gpibBus.cfg.rtmo);
    }
  }


  /***** Show or set end of send character *****/
  void eos_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 3, val)) return;
      gpibBus.cfg.eos = (uint8_t)val;
      if (isVerb) {
        dataPort.print(F("Set EOS to: "));
        dataPort.println(val);
      };
    } else {
      dataPort.println(gpibBus.cfg.eos);
    }
  }


  /***** Show or set EOI assertion on/off *****/
  void eoi_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 1, val)) return;
      gpibBus.cfg.eoi = val ? true : false;
      if (isVerb) {
        dataPort.print(F("Set EOI assertion: "));
        dataPort.println(val ? "ON" : "OFF");
      };
    } else {
      dataPort.println(gpibBus.cfg.eoi);
    }
  }


  /***** Show or set interface to controller/device mode *****/
  void cmode_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 1, val)) return;
      switch (val) {
        case 0:
          gpibBus.startDeviceMode();
          break;
        case 1:
          gpibBus.startControllerMode();
          break;
      }
      if (isVerb) {
        dataPort.print(F("Interface mode set to: "));
        dataPort.println(val ? "CONTROLLER" : "DEVICE");
      }
    } else {
      dataPort.println(gpibBus.isController());
    }
  }


  /***** Show or enable/disable sending of end of transmission character *****/
  void eot_en_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 1, val)) return;
      gpibBus.cfg.eot_en = val ? true : false;
      if (isVerb) {
        dataPort.print(F("Appending of EOT character: "));
        dataPort.println(val ? "ON" : "OFF");
      }
    } else {
      dataPort.println(gpibBus.cfg.eot_en);
    }
  }


  /***** Show or set end of transmission character *****/
  void eot_char_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 255, val)) return;
      gpibBus.cfg.eot_ch = (uint8_t)val;
      if (isVerb) {
        dataPort.print(F("EOT set to ASCII character: "));
        dataPort.println(val);
      };
    } else {
      dataPort.println(gpibBus.cfg.eot_ch, DEC);
    }
  }


  /***** Show or enable/disable auto mode *****/
  void amode_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 3, val)) return;
      if (val > 0 && isVerb) {
        dataPort.println(F("WARNING: automode ON can cause some devices to generate"));
        dataPort.println(F("         'addressed to talk but nothing to say' errors"));
      }
      gpibBus.cfg.amode = (uint8_t)val;
      if (gpibBus.cfg.amode < 3) autoRead = false;
      if (isVerb) {
        dataPort.print(F("Auto mode: "));
        dataPort.println(gpibBus.cfg.amode);
      }
    } else {
      dataPort.println(gpibBus.cfg.amode);
    }
  }


  /***** Display the controller version string *****/
  void ver_h(char *params) {
    // If "real" requested
    if (params != NULL && strncasecmp(params, "real", 3) == 0) {
      dataPort.println(F(FWVER));
      // Otherwise depends on whether we have a custom string set
    } else {
      if (strlen(gpibBus.cfg.vstr) > 0) {
        dataPort.println(gpibBus.cfg.vstr);
      } else {
        dataPort.println(F(FWVER));
      }
    }
  }


  /***** Address device to talk and read the sent data *****/
  void read_h(char *params) {
    // Clear read flags
    readWithEoi = false;
    readWithEndByte = false;
    endByte = 0;
    // Read any parameters
    if (params != NULL) {
      if (strlen(params) > 3) {
        if (isVerb) dataPort.println(F("Invalid parameter - ignored!"));
      } else if (strncasecmp(params, "eoi", 3) == 0) {  // Read with eoi detection
        readWithEoi = true;
      } else {  // Assume ASCII character given and convert to an 8 bit byte
        readWithEndByte = true;
        endByte = atoi(params);
      }
    }

    //DB_PRINT(F("readWithEoi:     "), readWithEoi);
    //DB_PRINT(F("readWithEndByte: "), readWithEndByte);

    if (gpibBus.cfg.amode == 3) {
      // In auto continuous mode we set this flag to indicate we are ready for continuous read
      autoRead = true;
    } else {
      // If auto mode is disabled we do a single read
      gpibBus.addressDevice(gpibBus.cfg.paddr, TALK);
      gpibBus.receiveData(dataPort, readWithEoi, readWithEndByte, endByte, GPIB_Buf, GPIB_length);
    }
  }


  /***** Send device clear (usually resets the device to power on state) *****/
  void clr_h() {
    if (gpibBus.sendSDC()) {
      if (isVerb) dataPort.println(F("Failed to send SDC"));
      return;
    }
    // Set GPIB controls back to idle state
    gpibBus.setControls(CIDS);
  }


  /***** Send local lockout command *****/
  void llo_h(char *params) {
    // NOTE: REN *MUST* be asserted (LOW)
    if (digitalRead(REN) == LOW) {
      // For 'all' send LLO to the bus without addressing any device
      // Devices will show REM as soon as they are addressed and need to be released with LOC
      if (params != NULL) {
        if (0 == strncmp(params, "all", 3)) {
          if (gpibBus.sendCmd(GC_LLO)) {
            if (isVerb) dataPort.println(F("Failed to send universal LLO."));
          }
        }
      } else {
        // Send LLO to currently addressed device
        if (gpibBus.sendLLO()) {
          if (isVerb) dataPort.println(F("Failed to send LLO!"));
        }
      }
    }
    // Set GPIB controls back to idle state
    gpibBus.setControls(CIDS);
  }


  /***** Send Go To Local (GTL) command *****/
  void loc_h(char *params) {
    // REN *MUST* be asserted (LOW)
    if (digitalRead(REN) == LOW) {
      if (params != NULL) {
        if (strncmp(params, "all", 3) == 0) {
          // Send request to clear all devices to local
          gpibBus.sendAllClear();
        }
      } else {
        // Send GTL to addressed device
        if (gpibBus.sendGTL()) {
          if (isVerb) dataPort.println(F("Failed to send LOC!"));
        }
        // Set GPIB controls back to idle state
        gpibBus.setControls(CIDS);
      }
    }
  }


  /***** Assert IFC for 150 microseconds *****/
  /* This indicates that the AR488 the Controller-in-Charge on
  * the bus and causes all interfaces to return to their idle 
  * state
  */
  void ifc_h() {
    if (gpibBus.cfg.cmode == 2) {
      // Assert IFC
      gpibBus.setControlVal(0b00000000, 0b00000001, 0);
      delayMicroseconds(150);
      // De-assert IFC
      gpibBus.setControlVal(0b00000001, 0b00000001, 0);
      if (isVerb) dataPort.println(F("IFC signal asserted for 150 microseconds"));
    }
  }


  /***** Send a trigger command *****/
  void trg_h(char *params) {
    char *param;
    uint8_t addrs[15] = { 0 };
    uint16_t val = 0;
    uint8_t cnt = 0;

    addrs[0] = addrs[0];  // Meaningless as both are zero but defaults compiler warning!

    // Read parameters
    if (params == NULL) {
      // No parameters - trigger addressed device only
      addrs[0] = gpibBus.cfg.paddr;
      cnt++;
    } else {
      // Read address parameters into array
      while (cnt < 15) {
        if (cnt == 0) {
          param = strtok(params, " \t");
        } else {
          param = strtok(NULL, " \t");
        }
        if (param == NULL) {
          break;  // Stop when there are no more parameters
        } else {
          if (notInRange(param, 1, 30, val)) return;
          addrs[cnt] = (uint8_t)val;
          cnt++;
        }
      }
    }

    // If we have some addresses to trigger....
    if (cnt > 0) {
      for (int i = 0; i < cnt; i++) {
        // Sent GET to the requested device
        if (gpibBus.sendGET(addrs[i])) {
          if (isVerb) dataPort.println(F("Failed to trigger device!"));
          return;
        }
      }

      // Set GPIB controls back to idle state
      gpibBus.setControls(CIDS);

      if (isVerb) dataPort.println(F("Group trigger completed."));
    }
  }


  /***** Reset the controller *****/
  /*
  * Arduinos can use the watchdog timer to reset the MCU
  * For other devices, we restart the program instead by
  * jumping to address 0x0000. This is not a hardware reset
  * and will not reset a crashed MCU, but it will re-start
  * the interface program and re-initialise all parameters. 
  */
  void rst_h() {
  #ifdef WDTO_1S
    // Where defined, reset controller using watchdog timeout
    unsigned long tout;
    tout = millis() + 2000;
    wdt_enable(WDTO_1S);
    while (millis() < tout) {};
    // Should never reach here....
    if (isVerb) {
      dataPort.println(F("Reset FAILED."));
    };
  #else
    // Otherwise restart program (soft reset)
    asm volatile("  jmp 0");
  #endif
  }


  /***** Serial Poll Handler *****/
  void spoll_h(char *params) {
    char *param;
    uint8_t addrs[15];
    uint8_t sb = 0;
    uint8_t r;
    //  uint8_t i = 0;
    uint8_t j = 0;
    uint16_t addrval = 0;
    bool all = false;
    bool eoiDetected = false;

    // Initialise address array
    for (int i = 0; i < 15; i++) {
      addrs[i] = 0;
    }

    // Read parameters
    if (params == NULL) {
      // No parameters - trigger addressed device only
      addrs[0] = gpibBus.cfg.paddr;
      j = 1;
    } else {
      // Read address parameters into array
      while (j < 15) {
        if (j == 0) {
          param = strtok(params, " \t");
        } else {
          param = strtok(NULL, " \t");
        }
        // No further parameters so exit
        if (!param) break;

        // The 'all' parameter given?
        if (strncmp(param, "all", 3) == 0) {
          all = true;
          j = 30;
          if (isVerb) dataPort.println(F("Serial poll of all devices requested..."));
          break;
          // Valid GPIB address parameter ?
        } else if (strlen(param) < 3) {  // No more than 2 characters
          if (notInRange(param, 1, 30, addrval)) return;
          addrs[j] = (uint8_t)addrval;
          j++;
          // Other condition
        } else {
          errBadCmd();
          if (isVerb) dataPort.println(F("Invalid parameter"));
          return;
        }
      }
    }

    // Send Unlisten [UNL] to all devices
    if (gpibBus.sendCmd(GC_UNL)) {
  #ifdef DEBUG_SPOLL
      DB_PRINT(F("failed to send UNL"), "");
  #endif
      return;
    }

    // Controller addresses itself as listner
    if (gpibBus.sendCmd(GC_LAD + gpibBus.cfg.caddr)) {
  #ifdef DEBUG_SPOLL
      DB_PRINT(F("failed to send LAD"), "");
  #endif
      return;
    }

    // Send Serial Poll Enable [SPE] to all devices
    if (gpibBus.sendCmd(GC_SPE)) {
  #ifdef DEBUG_SPOLL
      DB_PRINT(F("failed to send SPE"), "");
  #endif
      return;
    }

    // Poll GPIB address or addresses as set by i and j
    for (int i = 0; i < j; i++) {

      // Set GPIB address in val
      if (all) {
        addrval = i;
      } else {
        addrval = addrs[i];
      }

      // Don't need to poll own address
      if (addrval != gpibBus.cfg.caddr) {

        // Address a device to talk
        if (gpibBus.sendCmd(GC_TAD + addrval)) {

  #ifdef DEBUG_SPOLL
          DB_PRINT(F("failed to send TAD"), "");
  #endif
          return;
        }

        // Set GPIB control to controller active listner state (ATN unasserted)
        gpibBus.setControls(CLAS);

        // Read the response byte (usually device status) using handshake - suppress EOI detection
        r = gpibBus.readByte(&sb, false, &eoiDetected);

        // If we successfully read a byte
        if (!r) {
          if (j == 30) {
            // If all, return specially formatted response: SRQ:addr,status
            // but only when RQS bit set
            if (sb & 0x40) {
              dataPort.print(F("SRQ:"));
              dataPort.print(i);
              dataPort.print(F(","));
              dataPort.println(sb, DEC);
              // Exit on first device to respond
              i = j;
            }
          } else {
            // Return decimal number representing status byte
            dataPort.println(sb, DEC);
            if (isVerb) {
              dataPort.print(F("Received status byte ["));
              dataPort.print(sb);
              dataPort.print(F("] from device at address: "));
              dataPort.println(addrval);
            }
            // Exit on first device to respond
            i = j;
          }
        } else {
          if (isVerb) dataPort.println(F("Failed to retrieve status byte"));
        }
      }
    }
    if (all) dataPort.println();

    // Send Serial Poll Disable [SPD] to all devices
    if (gpibBus.sendCmd(GC_SPD)) {
  #ifdef DEBUG_SPOLL
      DB_PRINT(F("failed to send SPD"), "");
  #endif
      return;
    }

    // Send Untalk [UNT] to all devices
    if (gpibBus.sendCmd(GC_UNT)) {
  #ifdef DEBUG_SPOLL
      DB_PRINT(F("failed to send UNT"), "");
  #endif
      return;
    }

    // Unadress listners [UNL] to all devices
    if (gpibBus.sendCmd(GC_UNL)) {
  #ifdef DEBUG_SPOLL
      DB_PRINT(F("failed to send UNL"), "");
  #endif
      return;
    }

    // Set GPIB control to controller idle state
    gpibBus.setControls(CIDS);

    // Set SRQ to status of SRQ line. Should now be unasserted but, if it is
    // still asserted, then another device may be requesting service so another
    // serial poll will be called from the main loop
    /*  FLAG NO LONGER USED = NOW CHECKING STATUS OF LINE
    if (digitalRead(SRQ) == LOW) {
      isSRQ = true;
    } else {
      isSRQ = false;
    }
  */
    if (isVerb) dataPort.println(F("Serial poll completed."));
  }


  /***** Return status of SRQ line *****/
  void srq_h() {
    //NOTE: LOW=0=asserted, HIGH=1=unasserted
    //  dataPort.println(!digitalRead(SRQ));
    dataPort.println(gpibBus.isAsserted(SRQ));
  }


  /***** Set the status byte (device mode) *****/
  void stat_h(char *params) {
    uint16_t statusByte = 0;
    // A parameter given?
    if (params != NULL) {
      // Byte value given?
      if (notInRange(params, 0, 255, statusByte)) return;
      gpibBus.setStatus((uint8_t)statusByte);
    } else {
      // Return the currently set status byte
      dataPort.println(gpibBus.cfg.stat);
    }
  }


  /***** Save controller configuration *****/
  void save_h() {
  #ifdef E2END
    epWriteData(gpibBus.cfg.db, GPIB_CFG_SIZE);
    if (isVerb) dataPort.println(F("Settings saved."));
  #else
    dataPort.println(F("EEPROM not supported."));
  #endif
  }


  /***** Show state or enable/disable listen only mode *****/
  void lon_h(char *params) {
    uint16_t lval;
    if (params != NULL) {
      if (notInRange(params, 0, 1, lval)) return;
      isRO = lval ? true : false;
      if (isRO) {
        isTO = 0;        // Talk-only mode must be disabled!
        isProm = false;  // Promiscuous mode must be disabled!
      }
      if (isVerb) {
        dataPort.print(F("LON: "));
        dataPort.println(lval ? "ON" : "OFF");
      }
    } else {
      dataPort.println(isRO);
    }
  }


  /***** Show help message *****/
  void help_h(char *params) {
    char c, t;
    char token[20];
    uint8_t i;

    i = 0;
    for (size_t k = 0; k < strlen_P(cmdHelp); k++) {
      c = pgm_read_byte_near(cmdHelp + k);
      if (i < 20) {
        if (c == ':') {
          token[i] = 0;
          if ((params == NULL) || (strcmp(token, params) == 0)) {
            dataPort.print(F("++"));
            dataPort.print(token);
            dataPort.print(c);
            k++;
            t = pgm_read_byte_near(cmdHelp + k);
            dataPort.print(F(" ["));
            dataPort.print(t);
            dataPort.print(F("]"));
            i = 255;  // means we need to print until \n
          }

        } else {
          token[i] = c;
          i++;
        }
      } else if (i == 255) {
        dataPort.print(c);
      }
      if (c == '\n') {
        i = 0;
      }
    }
  }




  /***********************************/
  /***** CUSTOM COMMAND HANDLERS *****/
  /***********************************/

  /***** All serial poll *****/
  /*
  * Polls all devices, not just the currently addressed instrument
  * This is an alias wrapper for ++spoll all
  */
  void aspoll_h() {
    //  char all[4];
    //  strcpy(all, "all\0");
    spoll_h((char *)"all");
  }


  /***** Send Universal Device Clear *****/
  /*
  * The universal Device Clear (DCL) is unaddressed and affects all devices on the Gpib bus.
  */
  void dcl_h() {
    if (gpibBus.sendCmd(GC_DCL)) {
      if (isVerb) dataPort.println(F("Sending DCL failed"));
      return;
    }
    // Set GPIB controls back to idle state
    gpibBus.setControls(CIDS);
  }


  /***** Re-load default configuration *****/
  void default_h() {
    gpibBus.setDefaultCfg();
  }


  /***** Show or set end of receive character(s) *****/
  void eor_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 15, val)) return;
      gpibBus.cfg.eor = (uint8_t)val;
      if (isVerb) {
        dataPort.print(F("Set EOR to: "));
        dataPort.println(val);
      };
    } else {
      if (gpibBus.cfg.eor > 7) gpibBus.cfg.eor = 0;  // Needed to reset FF read from EEPROM after FW upgrade
      dataPort.println(gpibBus.cfg.eor);
    }
  }


  /***** Parallel Poll Handler *****/
  /*
  * Device must be set to respond on DIO line 1 - 8
  */
  void ppoll_h() {
    uint8_t sb = 0;

    // Poll devices
    // Start in controller idle state
    gpibBus.setControls(CIDS);
    delayMicroseconds(20);
    // Assert ATN and EOI
    gpibBus.setControlVal(0b00000000, 0b10010000, 0);
    //  setGpibState(0b10010000, 0b00000000, 0b10010000);
    delayMicroseconds(20);
    // Read data byte from GPIB bus without handshake
    sb = readGpibDbus();
    // Return to controller idle state (ATN and EOI unasserted)
    gpibBus.setControls(CIDS);

    // Output the response byte
    dataPort.println(sb, DEC);

    if (isVerb) dataPort.println(F("Parallel poll completed."));
  }


  /***** Assert or de-assert REN 0=de-assert; 1=assert *****/
  void ren_h(char *params) {
  #if defined(SN7516X) && not defined(SN7516X_DC)
    params = params;
    dataPort.println(F("Unavailable"));
  #else
    // char *stat;
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 1, val)) return;
      digitalWrite(REN, (val ? LOW : HIGH));
      if (isVerb) {
        dataPort.print(F("REN: "));
        dataPort.println(val ? "REN asserted" : "REN un-asserted");
      };
    } else {
      dataPort.println(digitalRead(REN) ? 0 : 1);
    }
  #endif
  }


  /***** Enable verbose mode 0=OFF; 1=ON *****/
  void verb_h() {
    isVerb = !isVerb;
    dataPort.print("Verbose: ");
    dataPort.println(isVerb ? "ON" : "OFF");
  }


  /***** Set version string *****/
  /* Replace the standard AR488 version string with something else
  *  NOTE: some instrument software requires a sepcific version string to ID the interface
  */
  void setvstr_h(char *params) {
    uint8_t plen;
    char idparams[64];
    plen = strlen(params);
    memset(idparams, '\0', 64);
    strncpy(idparams, "verstr ", 7);
    if (plen > 47) plen = 47;  // Ignore anything over 47 characters
    strncat(idparams, params, plen);

    id_h(idparams);

    /*  
    if (params != NULL) {
      len = strlen(params);
      if (len>47) len=47; 
      memset(AR488.vstr, '\0', 48);
      strncpy(AR488.vstr, params, len);
      if (isVerb) {
        dataPort.print(F("Changed version string to: "));
        dataPort.println(params);
      };
    }
  */
  }


  /***** Show state or enable/disable promiscuous mode *****/
  void prom_h(char *params) {
    uint16_t pval;
    if (params != NULL) {
      if (notInRange(params, 0, 1, pval)) return;
      isProm = pval ? true : false;
      if (isProm) {
        isTO = 0;      // Talk-only mode must be disabled!
        isRO = false;  // Listen-only mode must be disabled!
      }
      if (isVerb) {
        dataPort.print(F("PROM: "));
        dataPort.println(pval ? "ON" : "OFF");
      }
    } else {
      dataPort.println(isProm);
    }
  }



  /***** Talk only mode *****/
  void ton_h(char *params) {
    uint16_t toval;
    if (params != NULL) {
      if (notInRange(params, 0, 2, toval)) return;
      //    isTO = val ? true : false;
      isTO = (uint8_t)toval;
      if (isTO > 0) {
        isRO = false;    // Read-only mode must be disabled in TO mode!
        isProm = false;  // Promiscuous mode must be disabled in TO mode!
      }
    } else {
      if (isVerb) {
        dataPort.print(F("TON: "));
        switch (isTO) {
          case 1:
            dataPort.println(F("ON unbuffered"));
            break;
          case 2:
            dataPort.println(F("ON buffered"));
            break;
          default:
            dataPort.println(F("OFF"));
        }
      }
      dataPort.println(isTO);
    }
  }


  /***** SRQ auto - show or enable/disable automatic spoll on SRQ *****/
  /*
  * In device mode, when the SRQ interrupt is triggered and SRQ
  * auto is set to 1, a serial poll is conducted automatically
  * and the status byte for the instrument requiring service is
  * automatically returned. When srqauto is set to 0 (default)
  * an ++spoll command needs to be given manually to return
  * the status byte.
  */
  void srqa_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 1, val)) return;
      switch (val) {
        case 0:
          isSrqa = false;
          break;
        case 1:
          isSrqa = true;
          break;
      }
      if (isVerb) dataPort.println(isSrqa ? "SRQ auto ON" : "SRQ auto OFF");
    } else {
      dataPort.println(isSrqa);
    }
  }


  /***** Repeat a given command and return result *****/
  void repeat_h(char *params) {

    uint16_t count;
    uint16_t tmdly;
    char *param;

    if (params != NULL) {
      // Count (number of repetitions)
      param = strtok(params, " \t");
      if (strlen(param) > 0) {
        if (notInRange(param, 2, 255, count)) return;
      }
      // Time delay (milliseconds)
      param = strtok(NULL, " \t");
      if (strlen(param) > 0) {
        if (notInRange(param, 0, 30000, tmdly)) return;
      }

      // Pointer to remainder of parameters string
      param = strtok(NULL, "\n\r");
      if (strlen(param) > 0) {
        for (uint16_t i = 0; i < count; i++) {
          // Send string to instrument
          gpibBus.sendData(param, strlen(param));
          delay(tmdly);
          gpibBus.receiveData(dataPort, gpibBus.cfg.eoi, false, 0, GPIB_Buf, GPIB_length);
        }
      } else {
        errBadCmd();
        if (isVerb) dataPort.println(F("Missing parameter"));
        return;
      }
    } else {
      errBadCmd();
      if (isVerb) dataPort.println(F("Missing parameters"));
    }
  }


  /***** Run a macro *****/
  void macro_h(char *params) {
  #ifdef USE_MACROS
    uint16_t val;
    const char *macro;

    if (params != NULL) {
      if (notInRange(params, 0, 9, val)) return;
      //    execMacro((uint8_t)val);
      runMacro = (uint8_t)val;
    } else {
      for (int i = 0; i < 10; i++) {
        macro = (pgm_read_word(macros + i));
        //      dataPort.print(i);dataPort.print(F(": "));
        if (strlen_P(macro) > 0) {
          dataPort.print(i);
          dataPort.print(" ");
        }
      }
      dataPort.println();
    }
  #else
    memset(params, '\0', 5);
    dataPort.println(F("Disabled"));
  #endif
  }


  /***** Bus diagnostics *****/
  /*
  * Usage: xdiag mode byte
  * mode: 0=data bus; 1=control bus
  * byte: byte to write on the bus
  * Note: values to switch individual bits = 1,2,4,8,10,20,40,80
  * States revert to controller or device mode after 10 seconds
  * Databus reverts to 0 (all HIGH) after 10 seconds
  */
  void xdiag_h(char *params) {
    char *param;
    uint8_t mode = 0;
    uint8_t byteval = 0;

    // Get first parameter (mode = 0 or 1)
    param = strtok(params, " \t");
    if (param != NULL) {
      if (strlen(param) < 4) {
        mode = atoi(param);
        if (mode > 2) {
          dataPort.println(F("Invalid: 0=data bus; 1=control bus"));
          return;
        }
      }
    }
    // Get second parameter (8 bit byte)
    param = strtok(NULL, " \t");
    if (param != NULL) {
      if (strlen(param) < 4) {
        byteval = atoi(param);
      }

      switch (mode) {
        case 0:
          // Set to required value
          gpibBus.setDataVal(byteval);
          // Reset after 10 seconds
          delay(10000);
          gpibBus.setDataVal(0);
          break;
        case 1:
          // Set to required state
          gpibBus.setControlVal(0xFF, 0xFF, 1);      // Set direction (all pins as outputs)
          gpibBus.setControlVal(~byteval, 0xFF, 0);  // Set state (asserted=LOW so must invert value)
          // Reset after 10 seconds
          delay(10000);
          if (gpibBus.cfg.cmode == 2) {
            gpibBus.setControls(CINI);
          } else {
            gpibBus.setControls(DINI);
          }
          break;
      }
    }
  }


  /***** Enable Xon/Xoff handshaking for data transmission *****/
  /*
  void xonxoff_h(char *params){
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 1, val)) return;
      xonxoff = val ? true : false;
      if (isVerb) {
        dataPort.print(F("Xon/Xoff: "));
        dataPort.println(val ? "ON" : "OFF") ;
      }
    } else {
      dataPort.println(xonxoff);
    }
  }
  */


  /***** Set device ID *****/
  /*
  * Sets the device ID parameters including:
  * ++id verstr - version string (same as ++setvstr)
  * ++id name   - short name of device (e.g. HP3478A) up to 15 characters
  * ++id serial - serial number up to 9 digits long
  */
  void id_h(char *params) {
    uint8_t dlen = 0;
    char *keyword;  // Pointer to keyword following ++id
    char *datastr;  // Pointer to supplied data (remaining characters in buffer)
    char serialStr[10];

  #ifdef DEBUG_IDFUNC
    DB_PRINT(F("Params: "), params);
  #endif

    if (params != NULL) {
      keyword = strtok(params, " \t");
      datastr = keyword + strlen(keyword) + 1;
      dlen = strlen(datastr);
      if (dlen) {
        if (strncasecmp(keyword, "verstr", 6) == 0) {
  #ifdef DEBUG_IDFUNC
          DB_PRINT(F("Keyword: "), keyword);
          DB_PRINT(F("DataStr: "), datastr);
  #endif
          if (dlen > 0 && dlen < 48) {
  #ifdef DEBUG_IDFUNC
            DB_PRINT(F("Length OK"), "");
  #endif
            memset(gpibBus.cfg.vstr, '\0', 48);
            strncpy(gpibBus.cfg.vstr, datastr, dlen);
            if (isVerb) dataPort.print(F("VerStr: "));
            dataPort.println(gpibBus.cfg.vstr);
          } else {
            if (isVerb) dataPort.println(F("Length of version string must not exceed 48 characters!"));
            errBadCmd();
          }
          return;
        }
        if (strncasecmp(keyword, "name", 4) == 0) {
          if (dlen > 0 && dlen < 16) {
            memset(gpibBus.cfg.sname, '\0', 16);
            strncpy(gpibBus.cfg.sname, datastr, dlen);
          } else {
            if (isVerb) dataPort.println(F("Length of name must not exceed 15 characters!"));
            errBadCmd();
          }
          return;
        }
        if (strncasecmp(keyword, "serial", 6) == 0) {
          if (dlen < 10) {
            gpibBus.cfg.serial = atol(datastr);
          } else {
            if (isVerb) dataPort.println(F("Serial number must not exceed 9 characters!"));
            errBadCmd();
          }
          return;
        }
        //      errBadCmd();
      } else {
        if (strncasecmp(keyword, "verstr", 6) == 0) {
          dataPort.println(gpibBus.cfg.vstr);
          return;
        }
        if (strncasecmp(keyword, "fwver", 6) == 0) {
          dataPort.println(F(FWVER));
          return;
        }
        if (strncasecmp(keyword, "name", 4) == 0) {
          dataPort.println(gpibBus.cfg.sname);
          return;
        }
        void addr_h(char *params);
        if (strncasecmp(keyword, "serial", 6) == 0) {
          memset(serialStr, '\0', 10);
          snprintf(serialStr, 10, "%09lu", gpibBus.cfg.serial);  // Max str length = 10-1 i.e 9 digits + null terminator
          dataPort.println(serialStr);
          return;
        }
      }
    }
    errBadCmd();
  #ifdef DEBUG_IDFUNC
    DB_PRINT(F("done."), "");
  #endif
  }


  void idn_h(char *params) {
    uint16_t val;
    if (params != NULL) {
      if (notInRange(params, 0, 2, val)) return;
      gpibBus.cfg.idn = (uint8_t)val;
      if (isVerb) {
        dataPort.print(F("Sending IDN: "));
        dataPort.print(val ? "Enabled" : "Disabled");
        if (val == 2) dataPort.print(F(" with serial number"));
        dataPort.println();
      };
    } else {
      dataPort.println(gpibBus.cfg.idn, DEC);
    }
  }


  /***** Send device clear (usually resets the device to power on state) *****/
  void sendmla_h() {
    if (gpibBus.sendMLA()) {
      if (isVerb) dataPort.println(F("Failed to send MLA"));
      return;
    }
  }


  /***** Send device clear (usually resets the device to power on state) *****/
  void sendmta_h() {
    if (gpibBus.sendMTA()) {
      if (isVerb) dataPort.println(F("Failed to send MTA"));
      return;
    }
  }


  /***** Show or set read timout *****/
  void sendmsa_h(char *params) {
    uint16_t saddr;
    char *param;
    if (params != NULL) {
      // Secondary address
      param = strtok(params, " \t");
      if (strlen(param) > 0) {
        if (notInRange(param, 96, 126, saddr)) return;
        if (gpibBus.sendMSA(saddr)) {
          if (isVerb) dataPort.println(F("Failed to send MSA"));
          return;
        }
      }
      // Secondary address command parameter
      param = strtok(NULL, " \t");
      if (strlen(param) > 0) {
        gpibBus.setControls(CTAS);
        gpibBus.sendData(param, strlen(param));
        gpibBus.setControls(CLAS);
      }
      //    addressingSuppressed = true;
    }
  }


  /***** Send device clear (usually resets the device to power on state) *****/
  void unlisten_h() {
    if (gpibBus.sendUNL()) {
      if (isVerb) dataPort.println(F("Failed to send UNL"));
      return;
    }
    // Set GPIB controls back to idle state
    gpibBus.setControls(CIDS);
    //  addressingSuppressed = false;
  }


  /***** Send device clear (usually resets the device to power on state) *****/
  void untalk_h() {
    if (gpibBus.sendUNT()) {
      if (isVerb) dataPort.println(F("Failed to send UNT"));
      return;
    }
    // Set GPIB controls back to idle state
    gpibBus.setControls(CIDS);
    //  addressingSuppressed = false;
  }





  /******************************************************/
  /***** Device mode GPIB command handling routines *****/
  /******************************************************/

  /***** Attention handling routine *****/
  /*
  * In device mode is invoked whenever ATN is asserted
  */
  void attnRequired() {
    gpibBus.setControls(DLAS);

    const uint8_t cmdbuflen = 35;
    uint8_t cmdbytes[5] = { 0 };
    uint8_t db = 0;
    //  uint8_t rstat = 0;
    bool eoiDetected = false;
    uint8_t gpibcmd = 0;
    uint8_t bytecnt = 0;
    uint8_t atnstat = 0;  
    uint8_t ustat = 0;
    bool addressed = false;

  #ifdef EN_STORAGE
    uint8_t saddrcmd = 0;
  #endif

  #ifdef DEBUG_DEVICE_ATN
    uint8_t cmdbyteslist[cmdbuflen] = { 0 };
    uint8_t listbytecnt = 0;
  #endif

    // Set device listner active state (assert NDAC+NRFD (low), DAV=INPUT_PULLUP)
    //gpibBus.setControls(DLAS);

    /***** ATN read loop *****/
    // Read bytes received while ATN is asserted
    while ((gpibBus.isAsserted(ATN)) && (bytecnt < cmdbuflen)) {
      // Read the next byte from the bus, no EOI detection
      if (gpibBus.readByte(&db, false, &eoiDetected) > 0) {
        DB_PRINT(F("1 Break Attention read loop EZ."), "");
        break;
      }
      // Untalk or unlisten
      /*    

      if ( (db == 0x5F) || (db == 0x3F) ) {
      
        if (db == 0x3F) {
          if (device_unl_h()) ustat |= 0x01;
        }
        if (db == 0x5F) {
          if (device_unt_h()) ustat |= 0x02; 
        }
  */
      switch (db) {
        case 0x3F:
          ustat |= 0x01;
          DB_PRINT(F("2 Break Attention read loop EZ."), "");
          break;
        case 0x5F:
          ustat |= 0x02;
          DB_PRINT(F("3 Break Attention read loop EZ."), "");
          break;
        default:
          cmdbytes[bytecnt] = db;
          bytecnt++;
      }
  #ifdef DEBUG_DEVICE_ATN
      cmdbyteslist[listbytecnt] = db;
      listbytecnt++;
  #endif
    }

    // ATN read loop completed
    atnstat |= 0x01;

    /***** Try to unlisten bus *****/
  // if (ustat & 0x01) {
    //  DB_PRINT(F("try to unlisten EZ."), "");
    //  if (!device_unl_h()) ustat &= 0x01;  // Clears bit if UNL was not required
  // }

    /***** Try to untalk bus *****/
  // if (ustat & 0x01) {
  //   DB_PRINT(F("try to untalk EZ."), "");
  //   if (!device_unt_h()) ustat &= 0x02;  // Clears bit if UNT was not required
  //  }

    /***** Command process loop *****/
    // Process received addresses and command tokens
    if (bytecnt > 0) {

      // Process received command tokens
      for (uint8_t i = 0; i < bytecnt; i++) {

        if (!cmdbytes[i]) {
          DB_PRINT(F("4 Break Attention read loop EZ."), "");
          break;  // End loop on zero
        }

        db = cmdbytes[i];

        // Device is addressed to listen
        if (gpibBus.cfg.paddr == (db ^ 0x20)) {  // MLA = db^0x20
          atnstat |= 0x02;
          addressed = true;
          DB_PRINT(F("Condition 1 EZ."), "");
          gpibBus.setControls(DLAS);

          // Device is addressed to talk
        } else if (gpibBus.cfg.paddr == (db ^ 0x40)) {  // MLA = db^0x40
          // Call talk handler to send data
          atnstat |= 0x04;
          addressed = true;
          DB_PRINT(F("Condition 2 EZ."), "");
          gpibBus.setControls(DTAS);

  #ifdef EN_STORAGE
        } else if (db > 0x5F && db < 0x80) {
          // Secondary addressing command received
          saddrcmd = db;
          atnstat |= 0x10;
  #endif

        } else if (db < 0x20) {
          // Primary command received
          gpibcmd = db;
          atnstat |= 0x08;
        }
      }  // End for

    }  // ATN bytes processed


    /***** If we have not been adressed then back to idle and exit loop *****/
    if (!addressed) {
      gpibBus.setControls(DINI);
  #ifdef DEBUG_DEVICE_ATN
      showATNStatus(atnstat, ustat, cmdbyteslist, listbytecnt);
  #endif
      DB_PRINT(F("5 Break Attention read loop EZ."), "");
      return;
    }

    /***** If addressed, then perform the appropriate actions *****/
    if (addressed) {
  digitalWrite(GP_LED, LOW);      
      // If we have a primary GPIB command then execute it
      if (gpibcmd) {
        // Respond to GPIB command
        execGpibCmd(gpibcmd);
        // Clear flags
        gpibcmd = 0;
        atnstat |= 0x20;
  #ifdef DEBUG_DEVICE_ATN
        showATNStatus(atnstat, ustat, cmdbyteslist, listbytecnt);
  #endif
        DB_PRINT(F("6 Break Attention read loop EZ."), "");
        return;
      }

  #ifdef EN_STORAGE
      // If we have a secondary address then perform secondary GPIB command actions *****/
      if (addressed && saddrcmd) {
        // Execute the GPIB secondary addressing command
        storage.storeExecCmd(saddrcmd);
        // Clear secondary address command
        saddrcmd = 0;
        atnstat |= 0x40;
  #ifdef DEBUG_DEVICE_ATN
        showATNStatus(atnstat, ustat, cmdbyteslist, listbytecnt, rstat);
  #endif
        DB_PRINT(F("7 Break Attention read loop EZ."), "");
        return;
      }
  #endif

      // If no GPIB commands but addressed to listen then just listen
      if (gpibBus.isDeviceAddressedToListen()) {
        device_listen_h();
        atnstat |= 0x80;
        gpibBus.setControls(DIDS);
  #ifdef DEBUG_DEVICE_ATN
        DB_PRINT(F("Listen done."), "");
        showATNStatus(atnstat, ustat, cmdbyteslist, listbytecnt);
  #endif
        DB_PRINT(F("8 Break Attention read loop EZ."), "");
        return;
      }

      // If no GPIB commands but addressed to talk then send data
      if (gpibBus.isDeviceAddressedToTalk()) {
        device_talk_h();
        atnstat |= 0x80;
        gpibBus.setControls(DIDS);
  #ifdef DEBUG_DEVICE_ATN
        DB_PRINT(F("Talk done."), "");
        showATNStatus(atnstat, ustat, cmdbyteslist, listbytecnt);
  #endif
        DB_PRINT(F("9 Break Attention read loop EZ."), "");
        return;
      }
    }

  #ifdef DEBUG_DEVICE_ATN
    DB_PRINT(F("ATN: Nothing to process!"), "");
    showATNStatus(atnstat, ustat, cmdbyteslist, listbytecnt);
  #endif
  }


  #ifdef DEBUG_DEVICE_ATN
  void showATNStatus(uint8_t atnstat, uint8_t ustat, uint8_t atnbytes[], size_t bcnt) {

    if (ustat & 0x01) DB_PRINT(F("unlistened."), "");
    if (ustat & 0x02) DB_PRINT(F("untalked."), "");

    if (atnstat & 0x01) DB_PRINT(F("ATN read loop completed."), "");
    if (atnstat & 0x02) DB_PRINT(F("addressed to LISTEN."), "");
    if (atnstat & 0x04) DB_PRINT(F("addressed to TALK."), "");
    if (atnstat & 0x08) DB_PRINT(F("primary command received."), "");
    if (atnstat & 0x10) DB_PRINT(F("secondary command received."), "");
    if (atnstat & 0x20) DB_PRINT(F("primary command done."), "");
    if (atnstat & 0x40) DB_PRINT(F("secondary command done."), "");
    if (atnstat & 0x80) DB_PRINT(F("data transfer done."), "");

    DB_HEXA_PRINT(F("ATN bytes received: "), atnbytes, bcnt);
    DB_PRINT(bcnt, F(" ATN bytes read."));
    //  DB_PRINT(F("Error status: "),rstat);

    DB_PRINT(F("END attnReceived.\n\n"), "");
  }
  #endif


  /***** Execute GPIB command *****/
  void execGpibCmd(uint8_t gpibcmd) {

    // Respond to GPIB command
    switch (gpibcmd) {
      case GC_SPE:
        // Serial Poll enable request
  #ifdef DEBUG_DEVICE_ATN
        DB_PRINT(F("Received serial poll enable."), "");
  #endif
        device_spe_h();
        break;
      case GC_SPD:
        // Serial poll disable request
  #ifdef DEBUG_DEVICE_ATN
        DB_PRINT(F("Received serial poll disable."), "");
  #endif
        device_spd_h();
        break;
      case GC_UNL:
        // Unlisten
        device_unl_h();
        break;
      case GC_UNT:
        // Untalk
        device_unt_h();
        break;
      case GC_SDC:
        // Device clear (reset)
        device_sdc_h();
        break;
  #ifdef PIN_REMOTE
      case GC_LLO:
        // Remote lockout mode
        device_llo_h();
        break;
      case GC_GTL:
        // Local mode
        device_gtl_h();
        break;
  #endif
    }  // End switch
  }


  /***** Device is addressed to listen - so listen *****/
  void device_listen_h() {
    DB_PRINT("In device listen function EZ: ", "");
    // Receivedata params: stream, detectEOI, detectEndByte, endByte, *byte
    gpibBus.receiveData(dataPort, false, false, 0x0, GPIB_Buf, &GPIB_length);
    //AR_SERIAL_PORT.print("GPIB_Buf: ");
    // for (byte j=0; j < GPIB_length; j++) {
    //        AR_SERIAL_PORT.print((char) GPIB_Buf[j]);
    //      }
    //AR_SERIAL_PORT.println("");     
    //AR_SERIAL_PORT.println(GPIB_length);  
    respondGPIB(parse(GPIB_Buf));
  }


  /***** Device is addressed to talk - so send data *****/
  void device_talk_h() {
    DB_PRINT("LnRdy: ", lnRdy);
    DB_PRINT("Buffer: ", pBuf);
    //  if (lnRdy == 2) sendToInstrument(pBuf, pbPtr);
    if (lnRdy == 2) gpibBus.sendData(pBuf, pbPtr);
    // Flush the parse buffer and clear line ready flag
    flushPbuf();
    lnRdy = 0;
  }


  /***** Selected Device Clear *****/
  void device_sdc_h() {
    // If being addressed then reset
    if (isVerb) dataPort.println(F("Resetting..."));
  #ifdef DEBUG_DEVICE_ATN
    DB_PRINT(F("Reset adressed to me: "), "");
  #endif
    rst_h();
    if (isVerb) dataPort.println(F("Reset failed."));
  }


  /***** Serial Poll Disable *****/
  /***** Serial Poll Disable *****/
  void device_spd_h() {
  #ifdef DEBUG_DEVICE_ATN
    DB_PRINT(F("<- serial poll request ended."), "");
  #endif
    //  gpibBus.setDeviceAddressedState(DIDS);
    gpibBus.setControls(DIDS);
  }


  /***** Serial Poll Enable *****/
  void device_spe_h() {
  #ifdef DEBUG_DEVICE_ATN
    DB_PRINT(F("Serial poll request received from controller ->"), "");
  #endif
    gpibBus.sendStatus();
  #ifdef DEBUG_DEVICE_ATN
    DB_PRINT(F("Status sent: "), gpibBus.cfg.stat);
  #endif
    // Check if SRQ bit is set and clear it
    if (gpibBus.cfg.stat & 0x40) {
      gpibBus.setStatus(gpibBus.cfg.stat & ~0x40);
  #ifdef DEBUG_DEVICE_ATN
      DB_PRINT(F("SRQ bit cleared."), "");
  #endif
    }
  }


  /***** Unlisten *****/
  bool device_unl_h() {
    // Stop receiving and go to idle
    readWithEoi = false;
    // Immediate break - shouldn't ATN do this anyway?
    tranBrk = 3;  // Stop receving transmission
    // Clear addressed state flag and set controls to idle
    if (gpibBus.isDeviceAddressedToListen()) {
      gpibBus.setControls(DIDS);
      return true;
    }
    return false;
  }


  /***** Untalk *****/
  bool device_unt_h() {
    // Stop sending data and go to idle
    // Clear addressed state flag and set controls to listen
    if (gpibBus.isDeviceAddressedToTalk()) {
      gpibBus.setControls(DIDS);
      gpibBus.clearDataBus();
      return true;
    }
    return false;
  }


  #ifdef REMOTE_SIGNAL_PIN
  /***** Enabled remote mode *****/
  void device_llo_h() {
    digitalWrite(PIN_REMOTE, HIGH);
  }


  /***** Disable remote mode *****/
  void device_gtl_h() {
    digitalWrite(PIN_REMOTE, LOW);
  }
  #endif


  void lonMode() {

    uint8_t db = 0;
    uint8_t r = 0;
    bool eoiDetected = false;

    // Set bus for device read mode
    gpibBus.setControls(DLAS);

    while (isRO) {

      r = gpibBus.readByte(&db, false, &eoiDetected);
      if (r == 0) dataPort.write(db);

      // Check whether there are charaters waiting in the serial input buffer and call handler
      if (dataPort.available()) {

        lnRdy = serialIn_h();

        // We have a command return to main loop and execute it
        if (lnRdy == 1) break;

        // Clear the buffer to prevent it getting blocked
        if (lnRdy == 2) flushPbuf();
      }
    }

    // Set bus to idle
    gpibBus.setControls(DIDS);
  }


  /***** Talk only mpode *****/
  void tonMode() {

    // Set bus for device read mode
    gpibBus.setControls(DTAS);

    while (isTO > 0) {

      // Check whether there are charaters waiting in the serial input buffer and call handler
      if (dataPort.available()) {

        if (isTO == 1) {
          // Unbuffered version
          gpibBus.writeByte(dataPort.read(), false);
        }

        if (isTO == 2) {

          // Buffered version
          lnRdy = serialIn_h();

          // We have a command return to main loop and execute it
          if (lnRdy == 1) break;

          // Otherwise send the buffered data
          if (lnRdy == 2) {
            for (uint8_t i = 0; i < pbPtr; i++) {
              gpibBus.writeByte(pBuf[i], false);  // False = No EOI
            }
            flushPbuf();
          }
        }
      }
    }

    // Set bus to idle
    gpibBus.setControls(DIDS);
}

String parse(char *bytes)
{
  String response = ".";
  bool changeSR = false;
  flushSRbuf();
  if(strncmp(bytes, "PATHI01", 7) == 0)
  {    
    changeSR = true;
    SRbuf.pin.on7_6 = true;
    SRbuf.pin.on1_6 = true;
    SRbuf.pin.on15_2 = true;
  }
  if(strncmp(bytes, "PATHI02", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_6 = true;
    SRbuf.pin.on1_1 = true;
    SRbuf.pin.on16_2 = true;
  }  
  if(strncmp(bytes, "PATHI03", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_6 = true;
    SRbuf.pin.on1_2 = true;
    SRbuf.pin.on17_2 = true;
  }  
  if(strncmp(bytes, "PATHI04", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_1 = true;
    SRbuf.pin.on3_6 = true;
    SRbuf.pin.on18_2 = true;
  }
  if(strncmp(bytes, "PATHI05", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_1 = true;
    SRbuf.pin.on3_1 = true;
    SRbuf.pin.on19_2 = true;
  }
  if(strncmp(bytes, "PATHI06", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_1 = true;
    SRbuf.pin.on3_2 = true;
    SRbuf.pin.on20_2 = true;
  }
  if(strncmp(bytes, "PATHI07", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_2 = true;
    SRbuf.pin.on5_6 = true;
    SRbuf.pin.on21_2 = true;
  }
  if(strncmp(bytes, "PATHI08", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_2 = true;
    SRbuf.pin.on5_1 = true;
    SRbuf.pin.on22_2 = true;
  }
  if(strncmp(bytes, "PATHI09", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_2 = true;
    SRbuf.pin.on5_2 = true;
    SRbuf.pin.on23_2 = true;
  }
  if(strncmp(bytes, "PATHI10", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_6 = true;
    SRbuf.pin.on1_5 = true;
    SRbuf.pin.on24_2 = true;
  }
  if(strncmp(bytes, "PATHI11", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_6 = true;
    SRbuf.pin.on1_4 = true;
    SRbuf.pin.on25_2 = true;
  }
  if(strncmp(bytes, "PATHI12", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_6 = true;
    SRbuf.pin.on1_3 = true;
    SRbuf.pin.on26_2 = true;
  }
  if(strncmp(bytes, "PATHI13", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_1 = true;
    SRbuf.pin.on3_5 = true;
    SRbuf.pin.on27_2 = true;
  }
  if(strncmp(bytes, "PATHI14", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_1 = true;
    SRbuf.pin.on3_4 = true;
    SRbuf.pin.on28_2 = true;
  }
  if(strncmp(bytes, "PATHI15", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_1 = true;
    SRbuf.pin.on3_3 = true;
    SRbuf.pin.on29_2 = true;
  }
  if(strncmp(bytes, "PATHI16", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_2 = true;
    SRbuf.pin.on5_5 = true;
    SRbuf.pin.on30_2 = true;
  }
  if(strncmp(bytes, "PATHI17", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_2 = true;
    SRbuf.pin.on5_4 = true;
    SRbuf.pin.on31_2 = true;
  }
  if(strncmp(bytes, "PATHI18", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_2 = true;
    SRbuf.pin.on5_3 = true;
    SRbuf.pin.on32_2 = true;
  }
  if(strncmp(bytes, "PATHI19", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_5 = true;
    SRbuf.pin.on9_6 = true;
    SRbuf.pin.on33_2 = true;
  }
  if(strncmp(bytes, "PATHI20", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_5 = true;
    SRbuf.pin.on9_1 = true;
    SRbuf.pin.on34_2 = true;
  }
  if(strncmp(bytes, "PATHI21", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_5 = true;
    SRbuf.pin.on9_2 = true;
    SRbuf.pin.on35_2 = true;
  }
  if(strncmp(bytes, "PATHI22", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_4 = true;
    SRbuf.pin.on11_6 = true;
    SRbuf.pin.on36_2 = true;
  }
  if(strncmp(bytes, "PATHI23", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_4 = true;
    SRbuf.pin.on11_1 = true;
    SRbuf.pin.on37_2 = true;
  }
  if(strncmp(bytes, "PATHI24", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_4 = true;
    SRbuf.pin.on11_2 = true;
    SRbuf.pin.on38_2 = true;
  }
  if(strncmp(bytes, "PATHI25", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_3 = true;
    SRbuf.pin.on13_6 = true;
    SRbuf.pin.on39_2 = true;
  }
  if(strncmp(bytes, "PATHI26", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_3 = true;
    SRbuf.pin.on13_1 = true;
    SRbuf.pin.on40_2 = true;
  }
  if(strncmp(bytes, "PATHI27", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_3 = true;
    SRbuf.pin.on13_2 = true;
    SRbuf.pin.on41_2 = true;
  }
  if(strncmp(bytes, "PATHI28", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_5 = true;
    SRbuf.pin.on9_5 = true;
    SRbuf.pin.on42_2 = true;
  }
  if(strncmp(bytes, "PATHI29", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_5 = true;
    SRbuf.pin.on9_4 = true;
    SRbuf.pin.on43_2 = true;
  }
  if(strncmp(bytes, "PATHI30", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_5 = true;
    SRbuf.pin.on9_3 = true;
    SRbuf.pin.on44_2 = true;
  }
  if(strncmp(bytes, "PATHI31", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_4 = true;
    SRbuf.pin.on11_5 = true;
    SRbuf.pin.on45_2 = true;
  }
  if(strncmp(bytes, "PATHI32", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_4 = true;
    SRbuf.pin.on11_4 = true;
    SRbuf.pin.on46_2 = true;
  }
  if(strncmp(bytes, "PATHI33", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_4 = true;
    SRbuf.pin.on11_3 = true;
    SRbuf.pin.on47_2 = true;
  }
  if(strncmp(bytes, "PATHI34", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_3 = true;
    SRbuf.pin.on13_5 = true;
    SRbuf.pin.on48_2 = true;
  }
  if(strncmp(bytes, "PATHI35", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_3 = true;
    SRbuf.pin.on13_4 = true;
    SRbuf.pin.on49_2 = true;
  }
  if(strncmp(bytes, "PATHI36", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on7_3 = true;
    SRbuf.pin.on13_3 = true;
    SRbuf.pin.on50_2 = true;
  }
  if(strncmp(bytes, "PATHO01", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_6 = true;
    SRbuf.pin.on2_6 = true;
    SRbuf.pin.on15_1 = true;
  }
  if(strncmp(bytes, "PATHO02", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_6 = true;
    SRbuf.pin.on2_1 = true;
    SRbuf.pin.on16_1 = true;
  }  
  if(strncmp(bytes, "PATHO03", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_6 = true;
    SRbuf.pin.on2_2 = true;
    SRbuf.pin.on17_1 = true;
  }  
  if(strncmp(bytes, "PATHO04", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_1 = true;
    SRbuf.pin.on4_6 = true;
    SRbuf.pin.on18_1 = true;
  }
  if(strncmp(bytes, "PATHO05", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_1 = true;
    SRbuf.pin.on4_1 = true;
    SRbuf.pin.on19_1 = true;
  }
  if(strncmp(bytes, "PATHO06", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_1 = true;
    SRbuf.pin.on4_2 = true;
    SRbuf.pin.on20_1 = true;
  }
  if(strncmp(bytes, "PATHO07", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_2 = true;
    SRbuf.pin.on6_6 = true;
    SRbuf.pin.on21_1 = true;
  }
  if(strncmp(bytes, "PATHO08", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_2 = true;
    SRbuf.pin.on6_1 = true;
    SRbuf.pin.on22_1 = true;
  }
  if(strncmp(bytes, "PATHO09", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_2 = true;
    SRbuf.pin.on6_2 = true;
    SRbuf.pin.on23_1 = true;
  }
  if(strncmp(bytes, "PATHO10", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_6 = true;
    SRbuf.pin.on2_5 = true;
    SRbuf.pin.on24_1 = true;
  }
  if(strncmp(bytes, "PATHO11", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_6 = true;
    SRbuf.pin.on2_4 = true;
    SRbuf.pin.on25_1 = true;
  }
  if(strncmp(bytes, "PATHO12", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_6 = true;
    SRbuf.pin.on2_3 = true;
    SRbuf.pin.on26_1 = true;
  }
  if(strncmp(bytes, "PATHO13", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_1 = true;
    SRbuf.pin.on4_5 = true;
    SRbuf.pin.on27_1 = true;
  }
  if(strncmp(bytes, "PATHO14", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_1 = true;
    SRbuf.pin.on4_4 = true;
    SRbuf.pin.on28_1 = true;
  }
  if(strncmp(bytes, "PATHO15", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_1 = true;
    SRbuf.pin.on4_3 = true;
    SRbuf.pin.on29_1 = true;
  }
  if(strncmp(bytes, "PATHO16", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_2 = true;
    SRbuf.pin.on6_5 = true;
    SRbuf.pin.on30_1 = true;
  }
  if(strncmp(bytes, "PATHO17", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_2 = true;
    SRbuf.pin.on6_4 = true;
    SRbuf.pin.on31_1 = true;
  }
  if(strncmp(bytes, "PATHO18", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_2 = true;
    SRbuf.pin.on6_3 = true;
    SRbuf.pin.on32_1 = true;
  }
  if(strncmp(bytes, "PATHO19", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_5 = true;
    SRbuf.pin.on10_6 = true;
    SRbuf.pin.on33_1 = true;
  }
  if(strncmp(bytes, "PATHO20", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_5 = true;
    SRbuf.pin.on10_1 = true;
    SRbuf.pin.on34_1 = true;
  }
  if(strncmp(bytes, "PATHO21", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_5 = true;
    SRbuf.pin.on10_2 = true;
    SRbuf.pin.on35_1 = true;
  }
  if(strncmp(bytes, "PATHO22", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_4 = true;
    SRbuf.pin.on12_6 = true;
    SRbuf.pin.on36_1 = true;
  }
  if(strncmp(bytes, "PATHO23", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_4 = true;
    SRbuf.pin.on12_1 = true;
    SRbuf.pin.on37_1 = true;
  }
  if(strncmp(bytes, "PATHO24", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_4 = true;
    SRbuf.pin.on12_2 = true;
    SRbuf.pin.on38_1 = true;
  }
  if(strncmp(bytes, "PATHO25", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_3 = true;
    SRbuf.pin.on14_6 = true;
    SRbuf.pin.on39_1 = true;
  }
  if(strncmp(bytes, "PATHO26", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_3 = true;
    SRbuf.pin.on14_1 = true;
    SRbuf.pin.on40_1 = true;
  }
  if(strncmp(bytes, "PATHO27", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_3 = true;
    SRbuf.pin.on14_2 = true;
    SRbuf.pin.on41_1 = true;
  }
  if(strncmp(bytes, "PATHO28", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_5 = true;
    SRbuf.pin.on10_5 = true;
    SRbuf.pin.on42_1 = true;
  }
  if(strncmp(bytes, "PATHO29", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_5 = true;
    SRbuf.pin.on10_4 = true;
    SRbuf.pin.on43_1 = true;
  }
  if(strncmp(bytes, "PATHO30", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_5 = true;
    SRbuf.pin.on10_3 = true;
    SRbuf.pin.on44_1 = true;
  }
  if(strncmp(bytes, "PATHO31", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_4 = true;
    SRbuf.pin.on12_5 = true;
    SRbuf.pin.on45_1 = true;
  }
  if(strncmp(bytes, "PATHO32", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_4 = true;
    SRbuf.pin.on12_4 = true;
    SRbuf.pin.on46_1 = true;
  }
  if(strncmp(bytes, "PATHO33", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_4 = true;
    SRbuf.pin.on12_3 = true;
    SRbuf.pin.on47_1 = true;
  }
  if(strncmp(bytes, "PATHO34", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_3 = true;
    SRbuf.pin.on14_5 = true;
    SRbuf.pin.on48_1 = true;
  }
  if(strncmp(bytes, "PATHO35", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_3 = true;
    SRbuf.pin.on14_4 = true;
    SRbuf.pin.on49_1 = true;
  }
  if(strncmp(bytes, "PATHO36", 7) == 0)
  {
    changeSR = true;
    SRbuf.pin.on8_3 = true;
    SRbuf.pin.on14_3 = true;
    SRbuf.pin.on50_1 = true;
  }
  if(strncmp(bytes, "PATHPRESET", 10) == 0)
  {
    changeSR = true;
    SRbuf.pin.on1_6 = true;
    SRbuf.pin.on2_1 = true;
    SRbuf.pin.on3_6 = true;
    SRbuf.pin.on4_1 = true;
    SRbuf.pin.on5_6 = true;
    SRbuf.pin.on6_1 = true;
    SRbuf.pin.on7_1 = true;
    SRbuf.pin.on8_1 = true;
    SRbuf.pin.on9_6 = true;
    SRbuf.pin.on10_1 = true;
    SRbuf.pin.on11_6 = true;
    SRbuf.pin.on12_1 = true;
    SRbuf.pin.on13_6 = true;
    SRbuf.pin.on14_1 = true;
    SRbuf.pin.on15_1 = true;
    SRbuf.pin.on16_2 = true;
    SRbuf.pin.on17_1 = true;
    SRbuf.pin.on18_1 = true;
    SRbuf.pin.on19_2 = true;
    SRbuf.pin.on20_1 = true;
    SRbuf.pin.on21_1 = true;
    SRbuf.pin.on22_2 = true;
    SRbuf.pin.on23_1 = true;
    SRbuf.pin.on24_1 = true;
    SRbuf.pin.on25_1 = true;
    SRbuf.pin.on26_1 = true;
    SRbuf.pin.on27_1 = true;
    SRbuf.pin.on28_1 = true;
    SRbuf.pin.on29_1 = true;
    SRbuf.pin.on30_1 = true;
    SRbuf.pin.on31_1 = true;
    SRbuf.pin.on32_1 = true;
    SRbuf.pin.on33_1 = true;
    SRbuf.pin.on34_2 = true;
    SRbuf.pin.on35_1 = true;
    SRbuf.pin.on36_1 = true;
    SRbuf.pin.on37_2 = true;
    SRbuf.pin.on38_1 = true;
    SRbuf.pin.on39_1 = true;
    SRbuf.pin.on40_2 = true;
    SRbuf.pin.on41_1 = true;
    SRbuf.pin.on42_1 = true;
    SRbuf.pin.on43_1 = true;
    SRbuf.pin.on44_1 = true;
    SRbuf.pin.on45_1 = true;
    SRbuf.pin.on46_1 = true;
    SRbuf.pin.on47_1 = true;
    SRbuf.pin.on48_1 = true;
    SRbuf.pin.on49_1 = true;
    SRbuf.pin.on50_1 = true;
  }
  if(strncmp(bytes, "*IDN?", 5) == 0)
  {
    AR_SERIAL_PORT.println(" FOUND valid string");
    String manufacturer = "QUANTIC CORRY,";
    String model = "RFMS-2X36,";
    String serial = "20230209001,";
    String revision = "001";  
    response = manufacturer + model + serial + revision;
  }
  if(changeSR)
  {
    AR_SERIAL_PORT.println(" FOUND valid string");
    serialToParallel(&SRbuf.byte[0], sizeof(SRbuf));
    _delay_ms(25);
    flushSRindBuf();
    parallelToSerial(&SRindBuf.byte[0], sizeof(SRindBuf));
    if(compareBuf(&SRbuf.byte[0], &SRindBuf.byte[0], sizeof(SRbuf)))
    { 
      response = "OK.";
    }
  }
  *bytes = {0}; //clear buffer
  return response;
}

void respondGPIB(String response)
{
  flushPbuf();
  response.toCharArray(pBuf, PBSIZE);
  pbPtr = strlen(pBuf);
  lnRdy = 2;
}

void serialToParallel(byte *myDataOut, int numBytes)
 {
  // This shifts bits out MSB first on the rising edge of the clock, clock idles low
  //internal function setup
  int i=0;
  int pinState;
  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(SR1_IN, 0);
  digitalWrite(SPICLK1, 0);
  digitalWrite(L_CLK1, 0);
  //for each bit in the byte myDataOut&#xFFFD;
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights.
  for (i = (numBytes-1); i >= 0; i--)
  {
    for (int j = 7; j >= 0; j--) 
    {
      digitalWrite(SPICLK1, 0);
      //if the value passed to myDataOut and a bitmask result
      // true then... so if we are at i=6 and our value is
      // %11010100 it would the code compares it to %01000000
      // and proceeds to set pinState to 1.
      if (myDataOut[i] & (1<<j) ) 
      {
        pinState= 1;
      }
      else 
      {
        pinState= 0;
      }
      //Sets the pin to HIGH or LOW depending on pinState
      digitalWrite(SR1_IN, pinState);
      //register shifts bits on upstroke of clock pin
      digitalWrite(SPICLK1, 1);
      //zero the data pin after shift to prevent bleed through
      digitalWrite(SR1_IN, 0);
    }
  }
  //stop shifting
  digitalWrite(SR1_IN, 0);
  digitalWrite(SPICLK1, 0);
  digitalWrite(L_CLK1, 1);
  _delay_us(25);
  digitalWrite(L_CLK1, 0);
}

void parallelToSerial(byte *myDataIn, int numBytes)
 {
  //Parallel load then prepare shift register for bit shifting
  digitalWrite(PLb, 0);
  _delay_us(25);
  digitalWrite(L_CLK2, 1);
  digitalWrite(SR2_IN, 0);
  digitalWrite(SPICLK2, 0);
  digitalWrite(L_CLK2, 0);
  digitalWrite(PLb, 1);
  //for each bit in the byte myDataOut&#xFFFD;
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights.
  for (int i = (numBytes-1); i >= 0; i--)
  {
    for (int j = 7; j >= 0; j--) 
    {
      //register shifts bits on positive edge of clock pin
      //read when clock is low
      _delay_us(5);
      myDataIn[i] |= (digitalRead(SR2_OUT) << j);   
      digitalWrite(SPICLK2, 1);
      _delay_us(5);        
      digitalWrite(SPICLK2, 0);      
    }
  }
  //stop shifting
  digitalWrite(SPICLK2, 0);
  digitalWrite(L_CLK2, 0);
  digitalWrite(PLb, 1);
}

bool compareBuf(byte *bufMask, byte *buf, int bufSize)
{
  for (int i = 0; i < bufSize; i++)
  { 
    for (int j = 0; j < 8; j++) 
    {
      if((bufMask[i] & (1<<j)))   
      {
        if(!(buf[i] & (1<<j)))
        {
          AR_SERIAL_PORT.print("Check failed at byte ");
          AR_SERIAL_PORT.print(i);
          AR_SERIAL_PORT.print(" bit ");
          AR_SERIAL_PORT.println(j);
          return false;
        }
      }   
    }
  }
  return true;
}

void flushSRbuf()
{ 
  for(int i=0; i < sizeof(SRbuf); i++)
  {
    SRbuf.byte[i] = 0;
  }
}
void flushSRindBuf()
{ 
  for(int i=0; i < sizeof(SRindBuf); i++)
  {
    SRindBuf.byte[i] = 0;
  }
}

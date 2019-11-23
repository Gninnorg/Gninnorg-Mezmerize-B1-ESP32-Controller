#include <Arduino.h>
#include <screen.cpp>

/**********************************************************************************
**
**    VxD optical volume / preamp controller firmware
**
**    v1.0 07.2015
**
**
**    Uses code from: Malpartida (LCD lib), Hifiduino (IR remote), Adafruit (port expander), GreyGnome (PinChange)
**
**    Free for DIY. No commercial use.
**
**    2016-05-24 howarthcd
**    2017-11-23 JaTof - changes to support 2004 OLED display
**
***********************************************************************************/
#include <Wire.h>
#include "OLedI2C.h"
#include <EEPROM.h>
#include "Adafruit_MCP23008.h"
#include "PinChangeInt.h"
#include "IRLremote.h"


// Please only customize the values where the line comment starts with "//**" (unless you have a good reason)

// Debug mode increases memory usage, the code may not compile. In this case, reduce attenuation steps number to 30.
// !! COMMENT OUT THE FOLLOWING LINE WHEN NOT DEBUGGING ("//#define DEBUG") !!
//#define DEBUG


// Set I/O configuration here.
#define INPUTCOUNT 6    //** number of inputs, 0 to 6. If 0, comment out the next line.
char* inputName[INPUTCOUNT] = { "INPUT 1", "INPUT 2", "INPUT 3", "INPUT 4", "INPUT 5", "INPUT 6" }; //** each name maximum 9 characters. There must be exactly INPUTCOUNT names in the list.
#define ENABLEHTPASSTHROUGH false // Defines if Home Theather Pass Through is enabled. If true then the volume for a single input will be set to a fixed value (see below)
#define HTPASSTHROUGHINPUT 3 // Remember: The inputs are numbered from 0.
#define HTPASSTHROUGHVOLUME 20 // Must not be set to a number larger than VOL_MAX_STEP

// Specify the LCD size here
// Only 20x4 are supported
#define LCDCOLUMNS 20
#define LCDROWS 4

// Character to be used as a cursor
#define CURSORCHAR 31
#define DOTCHAR 221
#define INPUTCHAR 16
#define OUTPUTCHAR 17


/******* Text messages *******/
char msgWelcome1[] = "B1 Mezmerize";   //** <-- maximum 20 characters (line 1)
char msgWelcome2[] = "Muses volume";   //** <-- maximum 20 characters (line 2)
char msgWelcome3[] = "Carsten and Jan";     //** <-- maximum 20 characters (line 2)
char msgWelcome4[] = "built 2019";           //** <-- maximum 20 characters (line 2)

char msgSetup1[] = "Reserved";       //** <-- maximum 19 characters
char msgSetup2[] = "Reserved";      //** <-- maximum 19 characters
char msgSetup3[] = "Reserved";       //** <-- maximum 19 characters
char msgExit[] = "Exit";              //** <-- maximum 19 characters
char* setupName[4] = { msgSetup1, msgSetup2, msgSetup3, msgExit };

char msgErr[] = "ERROR ";             //** <-- maximum 17 characters. Last char must be blank

/******* Attenuator control *******/
#define VOL_MAX_STEP 64               //** maximum volume steps; range: 20...80. Higher = more memory usage
#define VOL_DEFAULT 20               //** default volume step. Must be <= than MAX

/******* SCREEN *******/
#define ROW_IN 0            //** LCD row to display input channel (0 - 3)

/******* TIMING *******/
#define TIME_LCDOFFAFTER 10   //** Time in seconds after last user input to turn LCD off
#define TIME_EXITSELECT 2     //** Time in seconds to exit I/O select mode when no activity

//------- do not edit below this point ----------------------------------------------------//

#define TIME_IGNOREREMOTE_VOL  100 // Time in millisec after IR VOLUME before repeating                          
#define TIME_IGNOREREMOTE_CMD  100 // Time in millisec after IR command before repeating
#define TIME_SWITCHBOUNCE 250  // Time in millisec to debounce switch
#define TIME_IRBOUNCE 200      // Time in millisec to debounce IR button
#define TIME_DACSETTLE 500     // Time in millisec to wait for the DAC to stabilize after setting output
#define TIME_RELAYLATCH 0    // Time in millisec for latching I/O relay driver to stabilize (0 if not using latching relays)

/******* I/O PORTS *******/
#define PIN_ENC1 2        // Encoder 1
#define PIN_ENC2 4        // Encoder 2
#define PIN_BTN 5         // Encoder switch
//#define PIN_LCDBRI 6      // LCD brightness control
#define PIN_REMOTE 7      // IR receiver (remote control) pin
#define PIN_EXT_R1 1      // port extender: relay 1
#define PIN_EXT_R2 2      // port extender: relay 2
#define PIN_EXT_R3 3      // port extender: relay 3
#define PIN_EXT_R4 4      // port extender: relay 4
#define PIN_EXT_R5 5      // port extender: relay 5
#define PIN_EXT_R6 6      // port extender: relay 6


/****** IR remote codes ******/

#define cIR_VOLDOWN 0x0    // Volume down
#define cIR_VOLUP   0x5    // Volume up
#define cIR_MUTE    0x6    // Mute
#define cIR_INPUT1  0x7    // Input 1
#define cIR_INPUT2  0x9    // Input 2
#define cIR_INPUT3  0x8    // Input 3
#define cIR_INPUT4  0xAA   // Input 4
#define cIR_INPUT5  0xAB   // Input 5
#define cIR_INPUT6  0xAC   // Input 6
#define cIR_REPEAT  0xFFFF // Repeat code from remote
#define cIR_UNKNOWN 0xAAAA // Unknown or no code - ignore input
#define NO_PORTB_PINCHANGES

/******* INTERFACE ********/
// Selection order in normal mode
#define SEL_INPUT 0

//Selection order in setup mode
#define SEL_BIAS 0
#define SEL_MEAS 1
#define SEL_CALIB 2
#define SEL_EXIT 3

// LCD

#define B 255
#define A 32


#if INPUTCOUNT == 0 && LCDCOLUMNS == 20
#define VOLCOL 6
#else
#define VOLCOL 11
#endif


#ifdef DEBUG
#define PRINT(x) Serial.print(x)     // console logging
#define PRINTLN(x) Serial.println(x)
#else
#define PRINT(x)
#define PRINTLN(x)
#endif


/******* MACHINE STATES *******/
#define STATE_RUN 0        // normal run state
#define STATE_INPUT_SELECT 1         // when user selects input/output
#define STATE_SETUP 2      // in setup menu mode
#define STATE_ERROR 3
#define STATE_OFF 4
#define ON HIGH
#define OFF LOW


///////////////////////////////////////////////////////////////////////////////////////
// VARIABLES

/******* GLOBAL *******/
Adafruit_MCP23008 mcp;    // port extender
byte volume;              // current volume, between 0 and VOL_STEPS
byte chan_in;             // current input channel. 0 based
byte state;               // current machine state
bool isMuted;             // current mute status
byte selInput;            // selected interface option in normal mode
byte selSetup;            // selected interface option in setup mode
byte errc;                // error code
bool btnReleased;
bool btnPressed;
bool IRPressed;
bool boot;
CNec IRLremote;

/******** LCD ********/
OLedI2C lcd;
byte LCDState = ON;
byte tick;
byte percent;

/*
** ------------------------- screen.cpp (start) ----------------------------
*/
void setLCDOn() {
  LCDState = ON;
  lcd.lcdOn();
}

void setLCDOff() {
  LCDState = OFF;
  lcd.lcdOff();
}

// prints a text on the LCD, padding with " " up to size
void lcd_print(char* text, byte size) {
	bool eot = false;
	for (byte i = 0; i < size; i++) {
		if (text[i] == 0)
			eot = true;
		if (eot)
			lcd.write(32);
		else
			lcd.write(text[i]);
	}
}

// prints progress bar
void printBar(byte p){
	if (p == 0) return;
	if (p > 100) p = 100;
	lcd.setCursor(int(((LCDCOLUMNS * 1.0) - 1.0)*p / 100.0), 1);
	lcd.write(31);
}

// 4x4 charset
const uint8_t cc0[8] = {    // Custom Character 0
  B00000,
  B00000,
  B00000,
  B00001,
  B00011,
  B00111,
  B01111,
  B11111
};

const uint8_t cc1[8] = {    // Custom Character 1
  B10000,
  B11000,
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111
};

const uint8_t cc2[8] = {    // Custom Character 2
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000
};

const uint8_t cc3[8] = {    // Custom Character 3
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

const uint8_t cc4[8] = {    // Custom Character 4
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00111,
  B00011,
  B00001
};

const uint8_t cc5[8] = {    // Custom Character 5
  B00001,
  B00011,
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111
};

const uint8_t cc6[8] = {    // Custom Character 6
  B00000,
  B00000,
  B00000,
  B10000,
  B11000,
  B11100,
  B11110,
  B11111
};

uint8_t cc7[8] = {    // Custom Character 7
  B11111,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100,
  B11000,
  B10000
};

//                    0                1                2                3                4                5                6                7                8                9
const char bn1[] = {  5,  2,  2,  1,  32,  5, 31, 32,   5,  2,  2,  1,   2,  2,  2,  1,  31, 32, 32, 31,  31,  2,  2,  2,   5,  2,  2,  2,   2,  2,  2, 31,   5,  2,  2,  1,   5,  2,  2,  1 };
const char bn2[] = { 31, 32, 32, 31,  32, 32, 31, 32,   0,  3,  3,  7,  32,  3,  3, 31,   4,  3,  3, 31,   4,  3,  3,  6,  31,  3,  3,  6,  32, 32,  0,  7,  31,  3,  3, 31,   4,  3,  3, 31 };
const char bn3[] = { 31, 32, 32, 31,  32, 32, 31, 32,  31, 32, 32, 32,  32, 32, 32, 31,  32, 32, 32, 31,  32, 32, 32, 31,  31, 32, 32, 31,  32, 32, 31, 32,  31, 32, 32, 31,  32, 32, 32, 31 };
const char bn4[] = {  4,  3,  3,  7,  32,  3, 31,  3,   4,  3,  3,  3,   4,  3,  3,  7,  32, 32, 32, 31,   4,  3,  3,  7,   4,  3,  3,  7,  32, 32, 31, 32,   4,  3,  3,  7,   4,  3,  3,  7 };

// Functions for printing two 4x4 digits. Works from 00-99
void printTwoNumber(byte column, byte number) {
  const byte firstdigit = (number / 10) * 4;
  const byte seconddigit = (number % 10) * 4;
  
  lcd.setCursor(column, 0); 
  lcd.sendData(bn1[firstdigit]);
  lcd.sendData(bn1[firstdigit + 1]);
  lcd.sendData(bn1[firstdigit + 2]);
  lcd.sendData(bn1[firstdigit + 3]);
  lcd.sendData(32); // Blank
  lcd.sendData(bn1[seconddigit]);
  lcd.sendData(bn1[seconddigit + 1]);
  lcd.sendData(bn1[seconddigit + 2]);
  lcd.sendData(bn1[seconddigit + 3]);

  lcd.setCursor(column, 1); 
  lcd.sendData(bn2[firstdigit]);
  lcd.sendData(bn2[firstdigit + 1]);
  lcd.sendData(bn2[firstdigit + 2]);
  lcd.sendData(bn2[firstdigit + 3]);
  lcd.sendData(32); // Blank
  lcd.sendData(bn2[seconddigit]);
  lcd.sendData(bn2[seconddigit + 1]);
  lcd.sendData(bn2[seconddigit + 2]);
  lcd.sendData(bn2[seconddigit + 3]);

  lcd.setCursor(column, 2); 
  lcd.sendData(bn3[firstdigit]);
  lcd.sendData(bn3[firstdigit + 1]);
  lcd.sendData(bn3[firstdigit + 2]);
  lcd.sendData(bn3[firstdigit + 3]);
  lcd.sendData(32); // Blank
  lcd.sendData(bn3[seconddigit]);
  lcd.sendData(bn3[seconddigit + 1]);
  lcd.sendData(bn3[seconddigit + 2]);
  lcd.sendData(bn3[seconddigit + 3]);

  lcd.setCursor(column, 3); 
  lcd.sendData(bn4[firstdigit]);
  lcd.sendData(bn4[firstdigit + 1]);
  lcd.sendData(bn4[firstdigit + 2]);
  lcd.sendData(bn4[firstdigit + 3]);
  lcd.sendData(32); // Blank
  lcd.sendData(bn4[seconddigit]);
  lcd.sendData(bn4[seconddigit + 1]);
  lcd.sendData(bn4[seconddigit + 2]);
  lcd.sendData(bn4[seconddigit + 3]);
}

void defineCustomChar() {
    lcd.createChar(0, cc0);  
    lcd.createChar(1, cc1);  
    lcd.createChar(2, cc2);  
    lcd.createChar(3, cc3);  
    lcd.createChar(4, cc4);  
    lcd.createChar(5, cc5);  
    lcd.createChar(6, cc6);  
    lcd.createChar(7, cc7);  
}
/*
** ------------------------- screen.cpp (slut) ----------------------------
*/


/******* REMOTE *******/
unsigned long mil_onRemote = 0; //debounce remote

unsigned int IRkey;            // The unique code of the remote key
unsigned int previousIRkey;    // The previous code (used for repeat)
bool isIRrepeat;      // Repeating code
byte lastInput = 0;   // Last selected input for A/B switching button (menu)
byte prevInput = 0;   // Previous
unsigned long mil_onRemoteKey = 0; // Stores time of last remote command


/******* ENCODER *******/
volatile int encoderPos = 0;       // encoder clicks


/******* TIMING *******/
unsigned long mil_onButton;    // Stores last time for switch debounce
unsigned long mil_onAction;    // Stores last time of user input
unsigned long mil_onFadeIn;    // LCD fade timing
unsigned long mil_onFadeOut;   // LCD fade timing
unsigned long mil_onSetLDR;    // Stores last time that a LDR has changed value
unsigned long mil_onInput;     // Last time relay set
unsigned long mil_onOutput;    // Last time relay set
unsigned long mil_delta;


//////////////////////////////////////////////////////////////////////////////////////
// RELAYS and I/O control

byte relayMap[6] = { PIN_EXT_R1, PIN_EXT_R2, PIN_EXT_R3, PIN_EXT_R4, PIN_EXT_R5, PIN_EXT_R6};

void setMute(byte volume) {
  isMuted = volume == 0;

  // CODE FOR MUSES TO SET MUTE

  printTwoNumber(VOLCOL, volume);
}

void setInput() {
  bool wasMuted = isMuted;
  if (millis() - mil_onInput > TIME_RELAYLATCH) {
    lcd.setCursor(1, ROW_IN);
    lcd_print(inputName[chan_in], (LCDCOLUMNS / 2) - 1);

    for (byte i = 0; i < INPUTCOUNT; i++)
      mcp.digitalWrite(relayMap[i], (chan_in == i));

    mil_onInput = millis();
  }
  if (isMuted && !wasMuted)
    if (ENABLEHTPASSTHROUGH && chan_in == HTPASSTHROUGHINPUT)
      setVolume(HTPASSTHROUGHVOLUME); // Volume is fixed on the selected input
    else
      setVolume(volume);
}


/* Rotary encoder handler for arduino.

  Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3.
  Contact: bb@cactii.net

  Quick implementation of rotary encoder routine.

  More info: http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html

*/

// Half-step mode?
//#define HALF_STEP
#define DIR_CCW 0x10
#define DIR_CW 0x20

#ifdef HALF_STEP
// Use the half-step state table (emits a code at 00 and 11)
const unsigned char ttable[6][4] = {
{0x3 , 0x2, 0x1,  0x0}, {0x23, 0x0, 0x1,  0x0},
{0x13, 0x2, 0x0,  0x0}, {0x3 , 0x5, 0x4,  0x0},
{0x3 , 0x3, 0x4, 0x10}, {0x3 , 0x5, 0x3, 0x20}
};
#else
// Use the full-step state table (emits a code at 00 only)       
const unsigned char ttable[7][4] = {
{0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x10},
{0x3, 0x2, 0x0, 0x0}, {0x3, 0x2, 0x1, 0x0},
{0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x20},
{0x6, 0x5, 0x4, 0x0},
};
#endif

volatile unsigned char encoderState = 0;

/* Read input pins and process for events. Call this either from a
  loop or an interrupt (eg pin change or timer).

  Returns 0 on no event, otherwise 0x80 or 0x40 depending on the direction.
*/
unsigned char rotary_process() {
  unsigned char pinstate = (digitalRead(PIN_ENC2) << 1) | digitalRead(PIN_ENC1);
  encoderState = ttable[encoderState & 0xf][pinstate];
  return (encoderState & 0x30);
}

//////////////////////////////////////////////////////////////////////////////////////
// IR Remote

// PENDING: Describe used library
unsigned int getIRkey() {
  if (IRLremote.available())
  {
    // Get the new data from the remote
    auto data = IRLremote.read();
    PRINT("Remote data.adress: "); PRINT(data.address); PRINT("Remote data.command: "); PRINTLN(data.command);
    if (data.address == 0x7984 && (data.command == cIR_VOLDOWN || data.command == cIR_VOLUP || data.command == cIR_MUTE || data.command == cIR_INPUT1 || data.command == cIR_INPUT2 || data.command == cIR_INPUT3 || data.command == cIR_INPUT4 || data.command == cIR_INPUT5 || data.command == cIR_INPUT6 ))
      return (data.command);
    else if (data.address == 0xFFFF && data.command == 0x0)
      return (cIR_REPEAT);
  }
  return (cIR_UNKNOWN);
}

///////////////////////////////////////////////////////////////////////////////////////
//  EEPROM routines

// address:
// 0  volume
// 1  input (0 based)

void saveValuesToEeprom() {
  if (EEPROM.read(0) != volume)
    EEPROM.write(0, volume);
  if (EEPROM.read(1) != chan_in)
    EEPROM.write(1, chan_in);
}

void loadValuesFromEeprom() {
  volume = EEPROM.read(0);
  chan_in = EEPROM.read(1);
  if (volume > VOL_MAX_STEP)
    volume = VOL_DEFAULT;
  if (chan_in >= INPUTCOUNT)
    chan_in = 0;
}


//void eraseEEPROM() {
//  for (byte i = 0; i < 1024; i++) //1024 max
//    EEPROM.write(i, 255);
//}


///////////////////////////////////////////////////////////////////////////////////////
//  Pin modes

void setPinModes() {
  Wire.begin();
  mcp.begin();
  mcp.pinMode(PIN_EXT_R1, OUTPUT);
  mcp.pinMode(PIN_EXT_R2, OUTPUT);
  mcp.pinMode(PIN_EXT_R3, OUTPUT);
  mcp.pinMode(PIN_EXT_R4, OUTPUT);
  mcp.pinMode(PIN_EXT_R5, OUTPUT);
  mcp.pinMode(PIN_EXT_R6, OUTPUT);

  pinMode(PIN_ENC1, INPUT_PULLUP);
  pinMode(PIN_ENC2, INPUT_PULLUP);
  pinMode(PIN_BTN, INPUT_PULLUP);

  pinMode(PIN_REMOTE, INPUT);
}

///////////////////////////////////////////////////////////////////////////////////////
//  state transitions

/** transition to setup mode, display menu **/
void toSetupState() {
  selSetup = SEL_BIAS;
  lcd.clear();
  lcd.write(CURSORCHAR); lcd.print(setupName[0]);
  lcd.setCursor(0, 1);
  lcd.write(DOTCHAR); lcd.print(setupName[1]);
  lcd.setCursor(0, 2);
  lcd.write(DOTCHAR); lcd.print(setupName[2]);
  lcd.setCursor(0, 3);
  lcd.write(DOTCHAR); lcd.print(setupName[3]);
  state = STATE_SETUP;
}

/** transition to error state **/
void toErrorState() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(msgErr); lcd.print(errc);
  state = STATE_ERROR;
}

/** transition to I/O channel adjust mode **/
void toIOState() {
  if (state != STATE_INPUT_SELECT) {
    if (INPUTCOUNT > 1) {
      selInput = SEL_INPUT;
      lcd.setCursor(0, ROW_IN); lcd.write(CURSORCHAR);
    }
  }
  state = STATE_INPUT_SELECT;
}

void storeLast() {
  if (chan_in != lastInput) {
    prevInput = lastInput;
    lastInput = chan_in;
    PRINT("chan:"); PRINT(chan_in); PRINT(" prev:"); PRINT(prevInput); PRINT(" LAST:"); PRINT(lastInput); PRINTLN();
  }
}

/** transition to normal volume adjust mode **/
void toRunState() {
  if (state != STATE_INPUT_SELECT) {
    lcd.clear();
    setVolume(volume);
#if INPUTCOUNT > 0
lcd.setCursor(1, ROW_IN);
lcd.print(inputName[chan_in]);
#endif
}

#if INPUTCOUNT > 0
lcd.setCursor(0, ROW_IN);
lcd.write(INPUTCHAR);
#endif

    mil_onAction = millis();
    state = STATE_RUN;
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  //  volume

  /******* set LDRs to current volume *******/
  void setVolume(byte vol) {

    if (ENABLEHTPASSTHROUGH && chan_in == HTPASSTHROUGHINPUT)
      vol = HTPASSTHROUGHVOLUME; // Volume is fixed on the selected input

    // CODE TO SET VOLUME ON MUSES HERE

  printTwoNumber(VOLCOL, vol);

  if (isMuted == vol || (isMuted && vol))
    setMute(vol);
}

void toggleMute() {
  if (isMuted)
    setVolume(volume);
  else
    setMute(0);
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//   SETUP

void setup() {
  setPinModes();

#ifdef DEBUG
Serial.begin(115200);
#endif

  PRINTLN("Starting");
  // display welcome message
  lcd.begin();
  lcd.clear();
  lcd.setCursor((LCDCOLUMNS + 1 - sizeof(msgWelcome1)) / 2, 0);
  lcd.print(msgWelcome1);
  lcd.setCursor((LCDCOLUMNS + 1 - sizeof(msgWelcome2)) / 2, 1);
  lcd.print(msgWelcome2);
  lcd.setCursor((LCDCOLUMNS + 1 - sizeof(msgWelcome3)) / 2, 2);
  lcd.print(msgWelcome3);
  lcd.setCursor((LCDCOLUMNS + 1 - sizeof(msgWelcome4)) / 2, 3);
  lcd.print(msgWelcome4);
  delay(1000);
  defineCustomChar();
  lcd.clear();

  loadValuesFromEeprom();
  state = STATE_INPUT_SELECT;
  setInput();
  isMuted = volume == 0;
  setVolume(volume);
  delay(250);
  setMute(volume);
  // Start reading the remote. PinInterrupt or PinChangeInterrupt* will automatically be selected
  IRLremote.begin(PIN_REMOTE);
  toRunState();

  mil_onAction = millis(); // begin counting to dim the LCD

  btnReleased = true;
  btnPressed = false;
  IRPressed = false;

  encoderPos = 0;
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//         LOOP

void loop() {

  // Detect power off
  if (state == STATE_RUN) {
    long vcc;
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2);
    ADCSRA |= _BV(ADSC);
    while (bit_is_set(ADCSRA, ADSC));
    vcc = ADCL;
    vcc |= ADCH << 8;
    vcc = 1126400L / vcc;
    if (vcc > 3000 && vcc < 4600) {
      //PRINT("power down...\n");                                // ***
      saveValuesToEeprom();
      state = STATE_OFF;
    }
  }


  /////////////////////////////////////////////////////////////////////////////////////
  // IR Remote

  if (millis() - mil_onRemote > TIME_IGNOREREMOTE_CMD && digitalRead(PIN_REMOTE) == LOW && (state == STATE_RUN || state == STATE_INPUT_SELECT)) {
    // Check if new IR protocol data is available
    IRkey = getIRkey();
    if (IRkey != cIR_UNKNOWN) {

      mil_onAction = millis();
      isIRrepeat = IRkey == cIR_REPEAT;

      // Prevent repeating if a code has not been received for a while.
      if ((millis() - mil_onRemoteKey) > 750)  {
        isIRrepeat = 0;
      }

      if (IRkey == cIR_VOLUP || IRkey == cIR_VOLDOWN || IRkey == cIR_MUTE || IRkey == cIR_INPUT1 || IRkey == cIR_INPUT2 || IRkey == cIR_INPUT3 || IRkey == cIR_INPUT4 || IRkey == cIR_INPUT5 || IRkey == cIR_INPUT6 ) {
        if (LCDState == OFF) setLCDOn();
      }

      if (isIRrepeat && (previousIRkey == cIR_VOLUP || previousIRkey == cIR_VOLDOWN))  // Repeat the specified keys
        IRkey = previousIRkey;
      else
        previousIRkey = IRkey;

      mil_onRemoteKey = millis();

      //PRINT("IR: "); PRINTLN(IRkey);

      switch (IRkey) {
        case cIR_VOLUP:  // volume up
          encoderPos++;
          break;
        case cIR_VOLDOWN:  // volume down
          encoderPos--;
          break;
        case cIR_INPUT1:
          if (INPUTCOUNT > 0) {
            chan_in = 0;
            setInput();
            setVolume(volume);
          }
          break;
        case cIR_INPUT2:
          if (INPUTCOUNT > 1) {
            chan_in = 1;
            setInput();
            setVolume(volume);
          }
          break;
        case cIR_INPUT3:
          if (INPUTCOUNT > 2) {
            chan_in = 2;
            setInput();
            setVolume(volume);
          }
          break;
        case cIR_INPUT4:
          if (INPUTCOUNT > 3) {
            chan_in = 3;
            setInput();
            setVolume(volume);
          }
        case cIR_INPUT5:
          if (INPUTCOUNT > 4) {
            chan_in = 4;
            setInput();
            setVolume(volume);
          }
        case cIR_INPUT6:
          if (INPUTCOUNT > 5) {
            chan_in = 5;
            setInput();
            setVolume(volume);
          }
          break;

        case cIR_MUTE:
          toggleMute();
          break;

      }// end SWITCH

      if (!isIRrepeat) {
        mil_onRemote = millis();
      }
    }
  }


  /////////////////////////////////////////////////////////////////////////////////////
  // Rotary encoder was turned

  unsigned char result = rotary_process();
  if (result) {
    if (result == DIR_CCW)
      encoderPos++;
    else
      encoderPos--;
  }
  if (encoderPos != 0)
  {
    mil_onAction = millis();
    if (LCDState == OFF) setLCDOn();

    /** encoder rotated in volume mode **/
    if (state == STATE_RUN) {
      if (!(ENABLEHTPASSTHROUGH == true && chan_in == HTPASSTHROUGHINPUT))
      {
        volume += encoderPos;
        if (volume > 250) // byte overflow...
          volume = 0;
        if (volume > VOL_MAX_STEP)
          volume = VOL_MAX_STEP;
        setVolume(volume);
      }
    }

    /** encoder rotated in channel select mode **/
    else if (state == STATE_INPUT_SELECT) {
      switch (selInput % 2) {
        case SEL_INPUT:
          if (INPUTCOUNT > 0) {
            chan_in += encoderPos;
            if (chan_in == 255)
              chan_in = INPUTCOUNT - 1;
            else
              chan_in %= INPUTCOUNT;
            setInput();
            setVolume(volume);
          }
          break;
      }
    }

    /** encoder rotated in "setup" mode **/
    else if (state == STATE_SETUP) {
      if (LCDState == OFF) setLCDOn();
      lcd.setCursor(0, selSetup); lcd.write(DOTCHAR);
      selSetup += encoderPos;
      selSetup %= 4;
      lcd.setCursor(0, selSetup); lcd.write(CURSORCHAR);
    }

    encoderPos = 0; // Reset the flag
  }  // End of encoder rotation

  /////////////////////////////////////////////////////////////////////////////////////
  // Button press

  btnPressed = digitalRead(PIN_BTN) == LOW && btnReleased;

  if (btnPressed) {

    btnReleased = false;
    mil_onButton = mil_onAction = millis();    // Start debounce timer
    if (LCDState == OFF) setLCDOn();

    /** button pressed in normal mode **/
    if (state == STATE_RUN)
      if (INPUTCOUNT <= 1) {
        if (btnPressed)
          toSetupState();
      }
      else
        toIOState();


    /** button pressed in IO mode **/
    else if (state == STATE_INPUT_SELECT) {
      if (btnPressed)
        toSetupState();  //switch to setup mode
      else
      {
        toRunState();
        setVolume(volume);
      }
    }

    /** button pressed in setup menu **/
    else if (btnPressed && state == STATE_SETUP) {
      switch (selSetup) {
        case SEL_BIAS:

          break;
        case SEL_MEAS:

          break;
        case SEL_CALIB:

          break;
        case SEL_EXIT:
          toRunState();
          break;
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////
  // timing

  //** timeout selection/menu
  if ((state == STATE_INPUT_SELECT) && millis() - mil_onAction > TIME_EXITSELECT * 1000) {
    saveValuesToEeprom();  // after modifying I/O, save to EEPROM
    toRunState();
  }

  //** encoder button debounce/release
  if (digitalRead(PIN_BTN) == HIGH && millis() - mil_onButton > TIME_SWITCHBOUNCE)
    btnReleased = true;

  //** turn off LCD if the set number of seconds have passed without activity
  if (state == STATE_RUN && LCDState == ON && millis() - mil_onAction > TIME_LCDOFFAFTER * 1000)
    setLCDOff();
}
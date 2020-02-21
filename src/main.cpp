/*
**
**    Controller for Mezmerize B1 Buffer using Muses7220 potentiometer 
**
**    Copyright (c) 2020 Carsten Grønning, Jan Abkjer Tofft
**
**    2019-2020
**
*/

#define VERSION 0.9

#include "Wire.h"
#include "OLedI2C.h"
#include "extEEPROM.h"
#include "ClickEncoder.h"
#include "TimerOne.h"
#include "IRLremote.h"
#include "Muses72320.h"
#include "MenuManager.h"
#include "MenuData.h"
#include "RelayController.h"

struct InputSettings
{
  bool Active;
  char Name[11];
  int MaxVol;
  int MinVol;
  int LastVol;
};

// This holds all the settings of the controller
// It is saved to the I2C EEPROM on the first run and read back into memory on subsequent runs
// The settings can be changed from the menu and the user can also chose to reset to default values if something goes wrong
// On startup of the controller it is checked if the EEPROM contains valid data by checking if the Version field equals the VERSION defined by the source code. If they are not the same default values will be written to EEPROM
// This is created as a union to be able to serialize/deserialize the data when writing and reading to/from the EEPROM
typedef union {
  struct
  {
    byte VolumeSteps;              // The number of steps of the volume control
    byte MinVolume;                // Minimum value allowed to be set by user
    byte MaxVolume;                // Maximum step allowed to be set by user
    byte MaxStartVolume;           // If StoreSetLevel is true, then limit the volume to the specified value when the controller is powered on
    byte MuteLevel;                // The level to be set when Mute is activated by the user. The Mute function of the Muses72320 is activated if 0 is specified
    bool StoreSetLevel;            // Remember/store the volume level for each separate input
    HashIR_data_t IR_UP;           // IR data to be interpreted as UP
    HashIR_data_t IR_UP_REPEAT;    // IR data to be interpreted as UP (if different code is sent when the UP key on the remote is held down)
    HashIR_data_t IR_DOWN;         // IR data to be interpreted as DOWN
    HashIR_data_t IR_DOWN_REPEAT;  // IR data to be interpreted as DOWN (if different code is sent when the DOWN key on the remote is held down)
    HashIR_data_t IR_LEFT;         // IR data to be interpreted as LEFT
    HashIR_data_t IR_RIGHT;        // IR data to be interpreted as RIGHT
    HashIR_data_t IR_SELECT;       // IR data to be interpreted as SELECT
    HashIR_data_t IR_BACK;         // IR data to be interpreted as BACK
    HashIR_data_t IR_MUTE;         // IR data to be interpreted as MUTE
    HashIR_data_t IR_ONOFF;        // IR data to be interpreted as ON/OFF - switch between running and suspend mode (and turn triggers off)
    HashIR_data_t IR_1;            // IR data to be interpreted as 1 (to select input 1 directly)
    HashIR_data_t IR_2;            // IR data to be interpreted as 2
    HashIR_data_t IR_3;            // IR data to be interpreted as 3
    HashIR_data_t IR_4;            // IR data to be interpreted as 4
    HashIR_data_t IR_5;            // IR data to be interpreted as 5
    HashIR_data_t IR_6;            // IR data to be interpreted as 6
    struct InputSettings Input[6]; // Settings for all 6 inputs
    bool Trigger1Active;           // false = the trigger is not active, true = the trigger is active
    bool Trigger1Type;             // false = momentary, true = latching
    bool Trigger1Mode;             // false = standard, true = intelligent (with measurement of NTC+LDR value)
    byte Trigger1OnDelay;          // seconds from controller power up to activation of trigger. The default delay allows time for the output relay of the Mezmerize to be activated before we turn on the power amps. The selection of an input of the Mezmerize will also be delayed.
    uint16_t Trigger1InactTimer;   // minutes before automatic power down (0 = never)
    byte Trigger1Temp;             // temperature protection: if the temperature is measured to the set number of degrees Celcius (via the LDRs), the controller will attempt to trigger a shutdown of the connected power amps (if set to 0, the temperature protection is not active
    bool Trigger2Active;           // false = the trigger is not active, true = the trigger is active
    bool Trigger2Type;             // false = momentary, true = latching
    bool Trigger2Mode;             // false = standard, true = intelligent (with measurement of NTC+LDR value)
    byte Trigger2OnDelay;          // seconds from controller power up to activation of trigger. The default delay allows time for the output relay of the Mezmerize to be activated before we turn on the power amps. The selection of an input of the Mezmerize will also be delayed.
    uint16_t Trigger2InactTimer;   // minutes before automatic power down (0 = never)
    byte Trigger2Temp;             // temperature protection: if the temperature is measured to the set number of degrees Celcius (via the LDRs), the controller will attempt to trigger a shutdown of the connected power amps (if set to 0, the temperature protection is not active)
    bool ScreenSaverActive;        // false = the display will stay on/not be dimmed, true = the display will be dimmed to the specified level after a specified period of time with no user input
    byte DisplayOnLevel;           // the contrast level of the display when it is on
    byte DisplayDimLevel;          // the contrast level of the display when screen saver is active. If DisplayOnLevel = 0xFF and DisplayDimLevel = 0x00 the display will be turned off when the screen saver is active (to reduce electrical noise)
    byte DisplayTimeout;           // number of seconds before the screen saver is activated.
    bool DisplaySelectedInput;     // false = the name of the active input is not shown on the display (ie. if only one input is used), true = the name of the selected input is shown on the display
    byte DisplayTemperature1;      // 0 = do not display the temperature measured by NTC 1, 1 = display in number of degrees Celcious, 2 = display as graphical representation, 3 = display both
    byte DisplayTemperature2;      // 0 = do not display the temperature measured by NTC 2, 1 = display in number of degrees Celcious, 2 = display as graphical representation, 3 = display both
    float Version;                 // used to check if data read from the EEPROM is valid with the compiled version of the compiled code - if not a reset to defaults is nessecary and they must be written to the EEPROM
  };
  byte data[]; // Allows us to be able to write/read settings from EEPROM byte-by-byte (to avoid specific serialization/deserialization code)
} Settings;

Settings CurrentSettings; // Holds all the current settings
void setCurrentSettingsToDefault(void);

byte CurrentInput = 0;      // The number of the currently set input
byte CurrentVolume = 0;     // The currently set volume
bool Muted = false;         // Indicates if we are in mute mode or not
byte prevSelectedInput = 0; // Holds the input selected before the current one
byte nextInput = 0;         // Used to find the next or previous active input (when KEY_RIGHT or KEY_LEFT is received)

// Setup Rotary encoders ------------------------------------------------------
ClickEncoder *encoder1 = new ClickEncoder(7, 8, 6, 4);
ClickEncoder::Button button1;
int16_t e1last, e1value;

ClickEncoder *encoder2 = new ClickEncoder(4, 5, 3, 4);
ClickEncoder::Button button2;
int16_t e2last, e2value;

void timerIsr()
{
  encoder1->service();
  encoder2->service();
}

void setupRotaryEncoders()
{
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
}

// Setup IR -------------------------------------------------------------------
#define pinIR 2
CHashIR IRLremote;

void setupIR()
{
  if (!IRLremote.begin(pinIR))
    Serial.println(F("You did not choose a valid pin."));
}

// Setup Relay Controller------------------------------------------------------
RelayController relayControl;
void setupRelayController()
{
  relayControl.begin();
}

// Setup EEPROM ---------------------------------------------------------------
#define EEPROM_Address 0x50
extEEPROM eeprom(kbits_64, 1, 32); // Set to use 24C64 Eeprom - if you use another type look in the datasheet for capacity in kbits (kbits_64) and page size in bytes (32)
void readSettingsFromEEPROM(void);
void writeSettingsToEEPROM(void);
void writeDefaultSettingsToEEPROM(void);

// Setup Display
OLedI2C lcd;
bool ScreenSaverIsOn = false;                  // Used to indicate whether the screen saver is running or not
unsigned long mil_onAction;                    // Used to keep track of the time of the last user interaction (part of the screen saver timing)
unsigned long mil_onRefreshTemperatureDisplay; // Used to time how often the display of temperatures is updated
#define TEMP_REFRESH_INTERVAL 30000            // Update the display of temperatures every 30 seconds

void setupDisplay()
{
  lcd.begin();
  lcd.backlight(CurrentSettings.DisplayOnLevel);
  lcd.clear();
  lcd.defineCustomChar();
}

//  Initialize the menu
enum AppModeValues
{
  APP_NORMAL_MODE,
  APP_MENU_MODE,
  APP_PROCESS_MENU_CMD
};

byte appMode = APP_NORMAL_MODE;

MenuManager Menu1(ctlMenu_Root, menuCount(ctlMenu_Root));

// Variables and predeclarations for editing Input names in menu
int arrowX;              // text edit arrow start X position on selection line
int arrowPointingUpDown; // text edit arrow start direction: up == 0, down == 1
String newInputName = "XXXXXXXXXX";
void startEditInputName(uint8_t InputNumber);
void moveArrowEditInputName(uint8_t Direction);
bool selectionInEditInputName(uint8_t InputNumber);
void endEditInputName();

void notImplementedYet(); // TO DO :-)

void drawMenu();
void refreshMenuDisplay(byte refreshMode);
byte menuIndex = 0;
byte processMenuCommand(byte cmdId);
byte getNavAction();

// Return null terminated string containing a specific number (count) of specified character (chr) ---
char *padc(char chr, unsigned char count)
{
  static char strbuf[LCD_COLS + 1];

  count = (count > LCD_COLS) ? LCD_COLS : count;

  int i;
  for (i = 0; i < count; i++)
  {
    strbuf[i] = chr;
  }
  strbuf[i] = 0;

  return strbuf;
}

// Return a string (*dest) that contains the contents of *str padded with a specific character up to a specific length (width but with a maximum of LCD_COLS)
char *rpad(char *dest, const char *str, char chr, unsigned char width)
{
  unsigned char len = strlen(str);

  width = width > LCD_COLS ? LCD_COLS : width;

  if (len < LCD_COLS && width > len)
  {
    strcpy(dest, str);
    strcat(dest, padc(chr, width - len));
  }
  else
  {
    strncpy(dest, str, width + 1);
  }
  return dest;
}

// Enumerated set of possible inputs from the user
enum UserInput
{
  KEY_NONE,        // No input
  KEY_UP,          // Rotary 1 turned CW or IR
  KEY_UP_REPEAT,   // IR
  KEY_DOWN,        // Rotary 1 turned CCW or IR
  KEY_DOWN_REPEAT, // IR
  KEY_SELECT,      // Rotary 1 switch pressed or IR
  KEY_RIGHT,       // Rotary 2 turned CW or IR
  KEY_LEFT,        // Rotary 2 turned CCW or IR
  KEY_BACK,        // Rotary 2 switch pressed or IR
  KEY_1,           // IR
  KEY_2,           // IR
  KEY_3,           // IR
  KEY_4,           // IR
  KEY_5,           // IR
  KEY_6,           // IR
  KEY_MUTE,        // IR
  KEY_ONOFF        // IR
};

byte UIkey; // holds the last received user input (from rotary encoders or IR)

// Returns input from the user - enumerated to be the same value no matter if input is from encoders or IR remote
byte getUserInput()
{
  byte receivedInput = KEY_NONE;

  // Read input from encoder 1
  e1value += encoder1->getValue();

  if (e1value != e1last)
  {
    if (e1value > e1last)
      receivedInput = KEY_UP;
    if (e1value < e1last)
      receivedInput = KEY_DOWN;
    e1last = e1value;
  }

  // Check if button on encoder 1 is clicked
  button1 = encoder1->getButton();
  switch (button1)
  {
  case ClickEncoder::Open:
    break;
  case ClickEncoder::Clicked:
    receivedInput = KEY_SELECT;
    break;
  default:
    break;
  }

  // Read input from encoder 2
  e2value += encoder2->getValue();

  if (e2value != e2last)
  {
    if (e2value > e2last)
      receivedInput = KEY_RIGHT;
    if (e2value < e2last)
      receivedInput = KEY_LEFT;
    e2last = e2value;
  }

  // Check if button on encoder 2 is clicked
  button2 = encoder2->getButton();
  switch (button2)
  {
  case ClickEncoder::Open:
    break;
  case ClickEncoder::Clicked:
    receivedInput = KEY_BACK;
    break;
  default:
    break;
  }

  // Check if any input from the IR remote
  if (IRLremote.available())
  {
    // Get the new data from the remote
    auto data = IRLremote.read();

    /* Print the protocol data
    Serial.print(F("Address: 0x"));
    Serial.println(data.address, HEX);
    Serial.print(F("Command: 0x"));
    Serial.println(data.command, HEX);
    */

    // Map the received IR input to UserInput values
    if (data.address == CurrentSettings.IR_UP.address && data.command == CurrentSettings.IR_UP.command)
      receivedInput = KEY_UP;
    else if (data.address == CurrentSettings.IR_UP_REPEAT.address && data.command == CurrentSettings.IR_UP_REPEAT.command)
      receivedInput = KEY_UP_REPEAT;
    else if (data.address == CurrentSettings.IR_DOWN.address && data.command == CurrentSettings.IR_DOWN.command)
      receivedInput = KEY_DOWN;
    else if (data.address == CurrentSettings.IR_DOWN_REPEAT.address && data.command == CurrentSettings.IR_DOWN_REPEAT.command)
      receivedInput = KEY_DOWN_REPEAT;
    else if (data.address == CurrentSettings.IR_LEFT.address && data.command == CurrentSettings.IR_LEFT.command)
      receivedInput = KEY_LEFT;
    else if (data.address == CurrentSettings.IR_RIGHT.address && data.command == CurrentSettings.IR_RIGHT.command)
      receivedInput = KEY_RIGHT;
    else if (data.address == CurrentSettings.IR_SELECT.address && data.command == CurrentSettings.IR_SELECT.command)
      receivedInput = KEY_SELECT;
    else if (data.address == CurrentSettings.IR_BACK.address && data.command == CurrentSettings.IR_BACK.command)
      receivedInput = KEY_BACK;
    else if (data.address == CurrentSettings.IR_MUTE.address && data.command == CurrentSettings.IR_MUTE.command)
      receivedInput = KEY_MUTE;
    else if (data.address == CurrentSettings.IR_ONOFF.address && data.command == CurrentSettings.IR_ONOFF.command)
      receivedInput = KEY_ONOFF;
    else if (data.address == CurrentSettings.IR_1.address && data.command == CurrentSettings.IR_1.command)
      receivedInput = KEY_1;
    else if (data.address == CurrentSettings.IR_2.address && data.command == CurrentSettings.IR_2.command)
      receivedInput = KEY_2;
    else if (data.address == CurrentSettings.IR_3.address && data.command == CurrentSettings.IR_3.command)
      receivedInput = KEY_3;
    else if (data.address == CurrentSettings.IR_4.address && data.command == CurrentSettings.IR_4.command)
      receivedInput = KEY_4;
    else if (data.address == CurrentSettings.IR_5.address && data.command == CurrentSettings.IR_5.command)
      receivedInput = KEY_5;
    else if (data.address == CurrentSettings.IR_6.address && data.command == CurrentSettings.IR_6.command)
      receivedInput = KEY_6;
  }

  // Turn Screen Saver on/off if it is activated and if no user input has been received during the defined number of seconds
  if (receivedInput == KEY_NONE && CurrentSettings.ScreenSaverActive)
  {
    if (!ScreenSaverIsOn && (millis() - mil_onAction > CurrentSettings.DisplayTimeout * 1000))
    {
      if (CurrentSettings.DisplayDimLevel == 0)
        lcd.lcdOff();
      else
        lcd.backlight(CurrentSettings.DisplayDimLevel);
      ScreenSaverIsOn = true;
    }
  }
  else
  {
    mil_onAction = millis();
    if (ScreenSaverIsOn)
    {
      if (CurrentSettings.DisplayDimLevel == 0)
        lcd.lcdOn();
      else
        lcd.backlight(CurrentSettings.DisplayOnLevel);
      ScreenSaverIsOn = false;
    }
  }

  return (receivedInput);
}

void reboot(void);
void DisplayTemperature(float Temp, float MaxTemp, byte ColumnForDegrees, byte StartRowForDegrees, byte ColumnForBar, byte StartRowForBar);

void setup()
{
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);

  Serial.begin(115200);
  Wire.begin();
  setupRotaryEncoders();
  setupIR();
  setupDisplay();
  setupRelayController();
  readSettingsFromEEPROM();
  if (CurrentSettings.Version != VERSION)
  {
    Serial.println(F("ERROR: Settings read from EEPROM are not ok"));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Restoring default"));
    lcd.setCursor(0, 1);
    lcd.print(F("settings..."));
    delay(2000);
    lcd.clear();
    writeDefaultSettingsToEEPROM();
    reboot();
  }
  else
  {
    Serial.println(F("Settings read from EEPROM are ok"));
    lcd.printTwoNumber(11, CurrentVolume);
    if (CurrentSettings.DisplaySelectedInput)
    {
      lcd.setCursor(0, 0);
      lcd.print(CurrentSettings.Input[CurrentInput].Name);
    }
    if (CurrentSettings.DisplayTemperature1)
      DisplayTemperature(relayControl.getTemperature(A0), CurrentSettings.Trigger1Temp, 0, 3, 0, 2);
    if (CurrentSettings.DisplayTemperature2)
      DisplayTemperature(relayControl.getTemperature(A1), CurrentSettings.Trigger2Temp, 5, 3, 5, 2);
    mil_onRefreshTemperatureDisplay = millis();
    Serial.println(F("Ready"));
  }
}

void DisplayTemperature(float Temp, float MaxTemp, byte ColumnForDegrees, byte StartRowForDegrees, byte ColumnForBar, byte StartRowForBar)
{
  // TO DO Implement CurrentSettings.displayTemp options (numerical, graphical, both)

  lcd.setCursor(ColumnForDegrees, StartRowForDegrees);
  if (Temp < 0)
  {
    lcd.print("OFF ");
    lcd.setCursor(ColumnForBar, StartRowForBar);
    lcd.print("AMP ");
  }
  else if (Temp > MaxTemp)
  {
    lcd.print("TEMP");
    lcd.setCursor(ColumnForBar, StartRowForBar);
    lcd.print("HIGH");
  }
  else
  {
    lcd.print(int(Temp));
    lcd.write(128); // Degree symbol

    // Map the range (0c ~ max temperature) to the range of the bar (0 to Number of characters to show the bar * Number of possible values per character )
    byte nb_columns = map(Temp, 0, MaxTemp, 0, 4 * 5);

    // Draw each character of the line
    lcd.setCursor(ColumnForBar, StartRowForBar);
    for (byte i = 0; i < 4; ++i) // Number of characters to show the bar = 4
    {

      // Write character depending on number of columns remaining to display
      if (nb_columns == 0)
      { // Case empty
        lcd.write(' ');
      }
      else if (nb_columns >= 5)
      {                 // Full box
        lcd.write(208); // Full box symbol
        nb_columns -= 5;
      }
      else
      {                                             // Partial box
        lcd.write(map(nb_columns, 1, 4, 212, 209)); // Map the remaining nb_columns (case between 1 and 4) to the corresponding character number symbols : 212 = 1 bar, 211 = 2 bars, 210 = 3 bars, 209 = 4 bars
        nb_columns = 0;
      }
    }
  }
}

void loop()
{
  UIkey = getUserInput();

  switch (appMode)
  {
  case APP_NORMAL_MODE:
    if (millis() > mil_onRefreshTemperatureDisplay + TEMP_REFRESH_INTERVAL)
    {
      mil_onRefreshTemperatureDisplay = millis();
      if (CurrentSettings.DisplayTemperature1)
        DisplayTemperature(relayControl.getTemperature(A0), CurrentSettings.Trigger1Temp, 0, 3, 0, 2);
      if (CurrentSettings.DisplayTemperature2)
        DisplayTemperature(relayControl.getTemperature(A1), CurrentSettings.Trigger2Temp, 5, 3, 5, 2);
    }
    switch (UIkey)
    {
    case KEY_NONE:
      break;
    case KEY_BACK:
      appMode = APP_MENU_MODE;
      menuIndex = 0;
      refreshMenuDisplay(REFRESH_DESCEND);
      break;
    case KEY_UP:
      // Turn volume up if we're not muted and we'll not exceed the global maximum volume or the maximum volume set for the currently selected input
      if (!Muted && (CurrentVolume < CurrentSettings.MaxVolume) && (CurrentVolume < CurrentSettings.Input[CurrentInput].MaxVol))
      {
        CurrentVolume++;
        CurrentSettings.Input[CurrentInput].LastVol = CurrentVolume;
        lcd.printTwoNumber(11, CurrentVolume);
        // TO DO Set volume to CurrentVolume
        // TO DO Save CurrentVolume to EEPROM
      }
      break;
    case KEY_DOWN:
      // Turn volume down if we're not muted and we'll not get below the global minimum volume or the minimum volume set for the currently selected input
      if (!Muted && (CurrentVolume > CurrentSettings.MinVolume) && (CurrentVolume > CurrentSettings.Input[CurrentInput].MinVol))
      {
        CurrentVolume--;
        CurrentSettings.Input[CurrentInput].LastVol = CurrentVolume;
        lcd.printTwoNumber(11, CurrentVolume);
        // TO DO Set volume to CurrentVolume
        // TO DO Save CurrentVolume to EEPROM
      }
      break;
    case KEY_RIGHT:
      // switch to next input
      prevSelectedInput = CurrentInput; // Save the current input as the previous selected input
      nextInput = CurrentInput + 1;
      if (nextInput > 5)
        nextInput = 0;
      while (!CurrentSettings.Input[nextInput].Active)
      {
        nextInput++;
        if (nextInput > 5)
          nextInput = 0;
      }
      if (CurrentInput != nextInput)
      {
        // TO DO Mute
        // TO DO switch to nextInput
        CurrentInput = nextInput;

        // TO DO set volume to saved volume for the CurrentSettings.Input[CurrentInput].LastVol (remember validations against global volume levels and local volume levels)
        // TO DO Unmute
        // TO DO Save to EEPROM
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[CurrentInput].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
        }
      }
      break;
    case KEY_LEFT:
      // switch to previous input
      prevSelectedInput = CurrentInput; // Save the current input as the previous selected input
      nextInput = CurrentInput - 1;
      if (CurrentInput == 0)
        nextInput = 5;
      while (!CurrentSettings.Input[nextInput].Active)
      {
        if (nextInput == 0)
          nextInput = 5;
        else
          nextInput--;
      }
      if (CurrentInput != nextInput)
      {
        // TO DO Mute
        // TO DO switch to nextInput
        CurrentInput = nextInput;

        // TO DO set volume to saved volume for the CurrentSettings.Input[CurrentInput].LastVol (remember validations against global volume levels and local volume levels)
        // TO DO Unmute
        // TO DO Save to EEPROM
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[CurrentInput].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
        }
      }
      break;
    case KEY_1:
      // Select input 1 (if active)
      if (CurrentSettings.Input[0].Active)
      {
        // TO DO switch to input 1
        CurrentInput = 0;
        // TO DO Save to EEPROM
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[0].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
        }
      }
      break;
    case KEY_2:
      // Select input 2 (if active)
      if (CurrentSettings.Input[1].Active)
      {
        // TO DO switch to input 2
        CurrentInput = 1;
        // TO DO Save to EEPROM
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[1].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
        }
      }
      break;
    case KEY_3:
      // Select input 3 (if active)
      if (CurrentSettings.Input[2].Active)
      {
        // TO DO switch to input 3
        CurrentInput = 2;
        // TO DO Save to EEPROM
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[2].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
        }
      }
      break;
    case KEY_4:
      // Select input 4 (if active)
      if (CurrentSettings.Input[3].Active)
      {
        // TO DO switch to input 4
        CurrentInput = 3;
        // TO DO Save to EEPROM
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[3].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
        }
      }
      break;
    case KEY_5:
      // Select input 5 (if active)
      if (CurrentSettings.Input[4].Active)
      {
        // TO DO switch to input 5
        CurrentInput = 4;
        // TO DO Save to EEPROM
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[4].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
        }
      }
      break;
    case KEY_6:
      // Select input 6 (if active)
      if (CurrentSettings.Input[5].Active)
      {
        // TO DO switch to input 6
        CurrentInput = 5;
        // TO DO Save to EEPROM
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[5].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
        }
      }
      break;
    case KEY_MUTE:
      // toggle mute
      if (Muted)
      {
        if (CurrentSettings.MuteLevel > 0)
        {
          // TODO Set volume to CurrentVolume
        }
        else
        {
          // TO DO Unmute the Muses volume control
        }
      }
      else
      {
        if (CurrentSettings.MuteLevel > 0)
        {
          // TO DO Set volume to CurrentSettings.MuteLevel
        }
        else
        {
          // TO DO Mute the Muses volume control
        }
      }
      break;
    }
    break;

  case KEY_ONOFF:
    // TO DO Suspend/standby state must be implemented
    break;

  case APP_MENU_MODE:
  { // Brackets to avoid warning: "jump to case label [-fpermissive]"
    byte menuMode = Menu1.handleNavigation(getNavAction, refreshMenuDisplay);

    if (menuMode == MENU_EXIT)
    {
      lcd.clear();
      // Back to APP_NORMAL_MODE
      lcd.printTwoNumber(11, CurrentVolume);
      lcd.setCursor(0, 0);
      if (CurrentSettings.DisplaySelectedInput)
        lcd.print(CurrentSettings.Input[CurrentInput].Name); // TO DO Need to add padding with spaces to delete earlier displayed input name
      mil_onRefreshTemperatureDisplay = millis();
      if (CurrentSettings.DisplayTemperature1)
        DisplayTemperature(relayControl.getTemperature(A0), CurrentSettings.Trigger1Temp, 0, 3, 0, 2);
      if (CurrentSettings.DisplayTemperature2)
        DisplayTemperature(relayControl.getTemperature(A1), CurrentSettings.Trigger2Temp, 5, 3, 5, 2);

      appMode = APP_NORMAL_MODE;
    }
    else if (menuMode == MENU_INVOKE_ITEM) // TO DO MENU_INVOKE_ITEM seems to be superfluous after my other changes
    {
      appMode = APP_PROCESS_MENU_CMD;
    }
    break;
  }
  case APP_PROCESS_MENU_CMD:

    byte processingComplete = processMenuCommand(Menu1.getCurrentItemCmdId());

    if (processingComplete)
    {
      appMode = APP_MENU_MODE;
      drawMenu();
    }
    break;
  }
}

//----------------------------------------------------------------------
// Addition or removal of menu items in MenuData.h will require this method
// to be modified accordingly.
byte processMenuCommand(byte cmdId)
{
  byte complete = false; // set to true when menu command processing complete.

  if (UIkey == KEY_SELECT)
  {
    complete = true;
  }

  switch (cmdId)
  {
  case mnuCmdVOL_STEPS:
  {
    // byte getByteNumber(currValue, min, max, "Description1", "Description2", "Description3")
    byte newVolSteps = CurrentSettings.VolumeSteps;
    lcd.clear();
    lcd.print("Volume");
    lcd.setCursor(0, 1);
    lcd.print("steps");
    lcd.printTwoNumber(11, newVolSteps);
    while (!complete)
    {
      switch (getUserInput())
      {
      case KEY_UP:
        // Increase volume steps
        if (newVolSteps < 99)
        {
          newVolSteps++;
          lcd.printTwoNumber(11, newVolSteps);
        }
        break;
      case KEY_DOWN:
        // Decrease volume steps
        if (newVolSteps > 1)
        {
          newVolSteps--;
          lcd.printTwoNumber(11, newVolSteps);
        }
        break;
      case KEY_SELECT:
        // Save new value
        CurrentSettings.VolumeSteps = newVolSteps;
        writeSettingsToEEPROM();
        complete = true;
        break;
      case KEY_BACK:
        // Exit without saving new value
        complete = true;
        break;
      default:
        break;
      }
    }
    break;
  }
  case mnuCmdMIN_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdMAX_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdMAX_START_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdMUTE_LVL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdSTORE_LVL:
    // TO DO
    // bool = getBool(currvalue, "Description1", "Description2", "Description3")
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_UP:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_DOWN:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_LEFT:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_RIGHT:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_SELECT:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_BACK:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_MUTE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_PREV:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_1:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_2:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_3:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_4:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_5:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdIR_6:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT1_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT1_NAME:
    startEditInputName(0);
    while (!complete)
    {
      switch (getUserInput())
      {
      case KEY_RIGHT:
        moveArrowEditInputName(KEY_RIGHT);
        break;
      case KEY_LEFT:
        moveArrowEditInputName(KEY_LEFT);
        break;
      case KEY_SELECT:
        if (selectionInEditInputName(0)) // Editing is done
          complete = true;
        break;
      case KEY_BACK:
        // Exit without saving new value
        complete = true;
        break;
      default:
        break;
      }
    }
    endEditInputName();
    break;
  case mnuCmdINPUT1_MAX_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT1_MIN_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT2_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT2_NAME:
    startEditInputName(1);
    while (!complete)
    {
      switch (getUserInput())
      {
      case KEY_RIGHT:
        moveArrowEditInputName(KEY_RIGHT);
        break;
      case KEY_LEFT:
        moveArrowEditInputName(KEY_LEFT);
        break;
      case KEY_SELECT:
        if (selectionInEditInputName(1)) // Editing is done
          complete = true;
        break;
      case KEY_BACK:
        // Exit without saving new value
        complete = true;
        break;
      default:
        break;
      }
    }
    endEditInputName();
    break;
  case mnuCmdINPUT2_MAX_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT2_MIN_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT3_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT3_NAME:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT3_MAX_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT3_MIN_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT4_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT4_NAME:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT4_MAX_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT4_MIN_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT5_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT5_NAME:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT5_MAX_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT5_MIN_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT6_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT6_NAME:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT6_MAX_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdINPUT6_MIN_VOL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER1_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER1_TYPE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER1_MODE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER1_ON_DELAY:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER1_INACT_TIMER:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER1_TEMP:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER2_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER2_TYPE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER2_MODE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER2_ON_DELAY:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER2_INACT_TIMER:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdTRIGGER2_TEMP:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdDISP_SAVER_ACTIVE:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdDISP_ON_LEVEL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdDISP_DIM_LEVEL:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdDISP_DIM_TIMEOUT:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdDISP_INPUT:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdDISP_TEMP1:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdDISP_TEMP2:
    // TO DO
    notImplementedYet();
    complete = true;
    break;
  case mnuCmdABOUT:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Firmware ");
    lcd.print(VERSION);
    lcd.setCursor(0, 1);
    lcd.print("built by carsten");
    lcd.write(160);
    lcd.setCursor(0, 2);
    lcd.print("groenning.net &");
    lcd.setCursor(0, 3);
    lcd.print("jan");
    lcd.write(160);
    lcd.print("tofft.dk (c)2020");
    delay(5000);
    complete = true;
    break;
  case mnuCmdRESET_NOW:
    // TO DO Maybe we need an extra confirmation from the user?
    writeDefaultSettingsToEEPROM();
    reboot();
    break;
  }

  return complete;
}

//----------------------------------------------------------------------
// Callback to convert button press to navigation action.
byte getNavAction()
{
  byte navAction = 0;

  if (UIkey == KEY_LEFT)
    navAction = MENU_ITEM_PREV;
  else if (UIkey == KEY_RIGHT)
    navAction = MENU_ITEM_NEXT;
  else if (UIkey == KEY_SELECT)
    navAction = MENU_ITEM_SELECT;
  else if (UIkey == KEY_BACK)
    navAction = MENU_BACK;
  return navAction;
}

void drawMenu()
{
  char strbuf[LCD_COLS + 1]; // one line of lcd display
  char nameBuf[LCD_COLS - 2];

  // Display the name of the menu
  lcd.setCursor(0, 0);
  if (Menu1.currentMenuHasParent())
  {
    rpad(strbuf, Menu1.getParentItemName(nameBuf), ' ', 20);
    lcd.print(strbuf);
  }
  else
    lcd.print(F("Main menu           "));

  // Clear any previously displayed arrow
  for (int i = 1; i < 4; i++)
  {
    lcd.setCursor(0, i);
    lcd.print("  ");
  }

  // Display the name of the currently active menu item on the row set by menuIndex (+1 because row 0 is used to display the name of the menu)
  lcd.setCursor(1, menuIndex + 1);
  lcd.write(16); // Mark with an arrow that this is the menu item that will be activated if the user press select
  rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
  lcd.print(strbuf);

  switch (menuIndex)
  {
  case 0:                                                            // The current menu item was displayed on row 0 - see if there is items to display on row 1 and 2
    if (Menu1.getCurrentItemIndex() + 1 <= Menu1.getMenuItemCount()) // Check if there is more menu items to display
    {
      Menu1.moveToNextItem();
      rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
      lcd.setCursor(2, menuIndex + 2);
      lcd.print(strbuf);
      if (Menu1.getCurrentItemIndex() + 2 <= Menu1.getMenuItemCount())
      {
        Menu1.moveToNextItem();
        rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
        lcd.setCursor(2, menuIndex + 3);
        lcd.print(strbuf);
        Menu1.moveToPreviousItem();
      }
      else
      {
        /* clear line at menuIndex + 3 */
        rpad(strbuf, " ", ' ', 17);
        lcd.setCursor(2, menuIndex + 3);
        lcd.print(strbuf);
      }
      Menu1.moveToPreviousItem();
    }
    else // No more items in the menu - make sure we clear up any previously displayed info from the display
    {
      /* clear line at menuIndex + 2 and menuIndex + 3*/
      rpad(strbuf, " ", ' ', 17);
      lcd.setCursor(2, menuIndex + 2);
      lcd.print(strbuf);
      lcd.setCursor(2, menuIndex + 3);
      lcd.print(strbuf);
    }

    break;
  case 1:                       // The current menu item was displayed on row 1 - display item on row 0 and see if there is an item to display on row 2
    Menu1.moveToPreviousItem(); // Move one item up in the menu to find the name of the item to be displayed one the line just before the current one
    rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
    lcd.setCursor(2, menuIndex);
    lcd.print(strbuf);
    Menu1.moveToNextItem(); // Move to next menu item to get back to current one
    if (Menu1.getCurrentItemIndex() + 1 <= Menu1.getMenuItemCount())
    {
      Menu1.moveToNextItem(); // Move to next menu item to find the name of the item after the current one
      rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
      lcd.setCursor(2, menuIndex + 2);
      lcd.print(strbuf);
      Menu1.moveToPreviousItem(); // Move to previous menu item to get back to current one
    }
    else // No more items in the menu - make sure we clear up any previously displayed info from the display
    {
      /* clear line at menuIndex + 2 */
      rpad(strbuf, " ", ' ', 17);
      lcd.setCursor(2, menuIndex + 2);
      lcd.print(strbuf);
    }
    break;
  case 2:                       // The current menu item was displayed on row 2 - display items on row 0 and 1
    Menu1.moveToPreviousItem(); // Move one item up in the menu to find the name of the item to be displayed one the line just before the current one
    rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
    lcd.setCursor(2, menuIndex);
    lcd.print(strbuf);
    Menu1.moveToPreviousItem(); // Once again we move one item up in the menu to find the name of the item to be displayed as the first one
    rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
    lcd.setCursor(2, menuIndex - 1);
    lcd.print(strbuf);
    Menu1.moveToNextItem(); // Move to next menu item
    Menu1.moveToNextItem(); // Move to next menu item to get back to current one
    break;
  }
}

// Callback to refresh display during menu navigation, using parameter of type enum DisplayRefreshMode.
void refreshMenuDisplay(byte refreshMode)
{
  // Display first line of the menu
  switch (refreshMode)
  {
  case REFRESH_MOVE_PREV: // user has navigated to previous menu item.
    if (menuIndex == 0)
      drawMenu();
    else
    {
      lcd.setCursor(1, menuIndex + 1);
      lcd.print(' '); // Delete the arrow previously set
      menuIndex--;
      // Redraw indication of what menu item is selected
      lcd.setCursor(1, menuIndex + 1);
      lcd.write(16); // Mark with an arrow that this is the menu item that will be activated if the user press select
    }
    break;
  case REFRESH_MOVE_NEXT: // user has navigated to next menu item.
    if (menuIndex == 2)
      drawMenu();
    else
    {
      lcd.setCursor(1, menuIndex + 1);
      lcd.print(' '); // Delete the arrow previously set
      menuIndex++;
      // Redraw indication of what menu item is selected
      lcd.setCursor(1, menuIndex + 1);
      lcd.write(16); // Mark with an arrow that this is the menu item that will be activated if the user press select
    }
    break;
  case REFRESH_ASCEND: // user has navigated to parent menu.
    menuIndex = 0;
    drawMenu();
    break;
  case REFRESH_DESCEND: // user has navigated to child menu.
    menuIndex = 0;
    drawMenu();
    break;
  }
}

// Called when an input name is to be edited
void startEditInputName(uint8_t InputNumber)
{
  // TO DO Maybe replace / character with symbol to allow for switching between upper and lower case letters?
  arrowX = 1;              // text edit arrow starts out at 'W'
  arrowPointingUpDown = 0; // text edit arrow starts out pointing down == 1; up == 0
  newInputName = "XXXXXXXXXX";
  lcd.clear();
  lcd.print("Input ");
  lcd.print(InputNumber + 1);
  lcd.setCursor(7, 0);
  lcd.write(223); // Right arrow
  lcd.setCursor(0, 1);
  lcd.write(byte(196));         // Print underscore to indicate Space
  for (int i = 65; i < 84; i++) // Print A-S
    lcd.write(i);
  lcd.setCursor(0, 3);
  for (int i = 84; i < 91; i++) // Print T-Z
    lcd.write(i);
  for (int i = 47; i < 58; i++) // Print /-9
    lcd.write(i);
  lcd.write(225);      // "Backspace" icon
  lcd.write(byte(28)); // "Enter" icon
  lcd.setCursor(arrowX, 2);
  lcd.write(byte(26 + arrowPointingUpDown)); // 26 is arrow up, 27 is arrow down
  newInputName = CurrentSettings.Input[InputNumber].Name;
  newInputName.trim();
  lcd.setCursor(9, 0);
  lcd.print(newInputName);
  lcd.setCursor(9 + newInputName.length(), 0);
}

void moveArrowEditInputName(uint8_t Direction)
{
  // Clear current arrow
  lcd.setCursor(arrowX, 2);
  lcd.write(' ');

  // Decide if position or direction of arrow must be changed
  if (arrowPointingUpDown == 0 && Direction == KEY_RIGHT)       // The arrow points up and the user input is "turn to the right"
    arrowPointingUpDown = 1;                            // Set the arrow to point down but don't change position of ArrowX
  else if (arrowPointingUpDown == 0 && Direction == KEY_LEFT) // The arrow points up and the user input is "turn to the left"
  {
    if (arrowX > 0)
      arrowX--; // Move arrow one postion to the left
  }
  else if (arrowPointingUpDown == 1 && Direction == KEY_RIGHT) // The arrow points down and the user input is "turn to the right"
  {
    if (arrowX < 19)
      arrowX++; // Move arrow one postion to the right
  }
  else if (arrowPointingUpDown == 1 && Direction == KEY_LEFT) // The arrow points down and the user input is "turn to the left"
    arrowPointingUpDown = 0;                            // Set the arrow to point up but don't change position of ArrowX

  // Display arrow
  lcd.setCursor(arrowX, 2);
  if (arrowPointingUpDown == 1) // if arrow == 1, then print arrow that points down; if arrow == 0, then print arrow that points up
    lcd.write(27);
  else
    lcd.write(26);
  lcd.setCursor(9 + newInputName.length(), 0);
}

// Called when the user selects a character or action like backspace while editing an Input name
bool selectionInEditInputName(uint8_t InputNumber)
{
  if (arrowPointingUpDown == 1) // If arrow points down
  {
    if (arrowX == 18) // Back Space (the Backspace icon has been selected)
    {
      if (newInputName.length() > 0) // Make sure there is a character to delete!
      {
        lcd.setCursor(9 + newInputName.length() - 1, 0);
        lcd.print(" "); // Print to clear the deleted character on the display
        newInputName = newInputName.substring(0, newInputName.length() - 1);

        lcd.setCursor(9 + newInputName.length(), 0);
      }
      return 0;
    }
    if (arrowX == 19) // Done editing (the Enter icon has been selected)
    {
      newInputName.trim();
      if (newInputName == "") // If no characters in new name then reset to original name
        newInputName = CurrentSettings.Input[InputNumber].Name;
      else
      {
        // Save new name to CurrentSettings
        for (uint8_t i = 0; i < newInputName.length(); i++)
          CurrentSettings.Input[InputNumber].Name[i] = newInputName.charAt(i);
        // Pad Name with spaces - makes it easier to display
        for (uint8_t i = newInputName.length(); i < 10; i++)
          CurrentSettings.Input[InputNumber].Name[i] = ' ';
        CurrentSettings.Input[InputNumber].Name[10] = '\0';
        // Save to EEPROM
        writeSettingsToEEPROM();
      }
      return 1;
    }
    if (newInputName.length() < 10) // Only allow up to 10 characters
    {
      byte v = 84;
      if (arrowX > 5)
        v = 40;
      newInputName = newInputName + (char(arrowX + v));
    }
  }
  else // Arrow points down
  {
    if (newInputName.length() < 10) // Only allow up to 10 characters
    {
      if (arrowX == 0) // Space character has been selected (though it is shown as underscore)
        newInputName = newInputName + " ";
      else // A character between A-S has been selected, so add it to the string
        newInputName = newInputName + (char(arrowX + 64));
    }
  }
  lcd.setCursor(9, 0);
  lcd.print(newInputName);
  return 0;
}

// Called when an input name has been edited
void endEditInputName()
{
  // TO DO Blinking cursor in EditInputName
  lcd.BlinkingCursorOff();
  lcd.clear();
}

void notImplementedYet()
{
  lcd.clear();
  lcd.print("This function is");
  lcd.setCursor(0, 1);
  lcd.print("not implemented yet.");
  lcd.setCursor(0, 2);
  lcd.print("Press SELECT to");
  lcd.setCursor(0, 3);
  lcd.print("continue...");
  while (getUserInput() != KEY_SELECT)
  {
  };
}

// Loads default settings into CurrentSettings - this is only done when the EEPROM does not contain valid settings or when reset is chosen by user in the menu
void setCurrentSettingsToDefault()
{
  CurrentSettings.VolumeSteps = 24;
  CurrentSettings.MinVolume = 0;
  CurrentSettings.MaxVolume = CurrentSettings.VolumeSteps;
  CurrentSettings.MaxStartVolume = CurrentSettings.VolumeSteps;
  CurrentSettings.MuteLevel = 0;
  CurrentSettings.StoreSetLevel = true;
  CurrentSettings.IR_UP.address = 0x24;
  CurrentSettings.IR_UP.command = 0x3AEA5A5F;
  CurrentSettings.IR_UP_REPEAT.address = 0x24;
  CurrentSettings.IR_UP_REPEAT.command = 0x3AEA5A5F;
  CurrentSettings.IR_DOWN.address = 0x24;
  CurrentSettings.IR_DOWN.command = 0xE64E6057;
  CurrentSettings.IR_DOWN_REPEAT.address = 0x24;
  CurrentSettings.IR_DOWN_REPEAT.command = 0xE64E6057;
  CurrentSettings.IR_LEFT.address = 0x24;
  CurrentSettings.IR_LEFT.command = 0x4C7A8423;
  CurrentSettings.IR_RIGHT.address = 0x24;
  CurrentSettings.IR_RIGHT.command = 0xA1167E2B;
  CurrentSettings.IR_SELECT.address = 0x24;
  CurrentSettings.IR_SELECT.command = 0x91998CA3;
  CurrentSettings.IR_BACK.address = 0x24;
  CurrentSettings.IR_BACK.command = 0xE28395C7;
  CurrentSettings.IR_MUTE.address = 0x24;
  CurrentSettings.IR_MUTE.command = 0x41C09D23;
  CurrentSettings.IR_ONOFF.address = 0x24;
  CurrentSettings.IR_ONOFF.command = 0x41D976CF;
  CurrentSettings.IR_1.address = 0x24;
  CurrentSettings.IR_1.command = 0xC43587C7;
  CurrentSettings.IR_2.address = 0x24;
  CurrentSettings.IR_2.command = 0x6F998DBF;
  CurrentSettings.IR_3.address = 0x24;
  CurrentSettings.IR_3.command = 0xB9947A73;
  CurrentSettings.IR_4.address = 0x24;
  CurrentSettings.IR_4.command = 0x64F8806B;
  CurrentSettings.IR_5.address = 0x24;
  CurrentSettings.IR_5.command = 0x1FC09E3F;
  CurrentSettings.IR_6.address = 0x24;
  CurrentSettings.IR_6.command = 0xCB24A437;
  CurrentSettings.Input[0].Active = true;
  strcpy(CurrentSettings.Input[0].Name, "Input 1   ");
  CurrentSettings.Input[0].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[0].MinVol = 0;
  CurrentSettings.Input[0].LastVol = 0;
  CurrentSettings.Input[1].Active = true;
  strcpy(CurrentSettings.Input[1].Name, "Input 2   ");
  CurrentSettings.Input[1].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[1].MinVol = 0;
  CurrentSettings.Input[1].LastVol = 0;
  CurrentSettings.Input[2].Active = true;
  strcpy(CurrentSettings.Input[2].Name, "Input 3   ");
  CurrentSettings.Input[2].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[2].MinVol = 0;
  CurrentSettings.Input[2].LastVol = 0;
  CurrentSettings.Input[3].Active = true;
  strcpy(CurrentSettings.Input[3].Name, "Input 4   ");
  CurrentSettings.Input[3].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[3].MinVol = 0;
  CurrentSettings.Input[3].LastVol = 0;
  CurrentSettings.Input[4].Active = true;
  strcpy(CurrentSettings.Input[4].Name, "Input 5   ");
  CurrentSettings.Input[4].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[4].MinVol = 0;
  CurrentSettings.Input[4].LastVol = 0;
  CurrentSettings.Input[5].Active = true;
  strcpy(CurrentSettings.Input[5].Name, "Input 6   ");
  CurrentSettings.Input[5].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[5].MinVol = 0;
  CurrentSettings.Input[5].LastVol = 0;
  CurrentSettings.Trigger1Type = false;
  CurrentSettings.Trigger1Mode = true;
  CurrentSettings.Trigger1OnDelay = 10;
  CurrentSettings.Trigger1InactTimer = 0;
  CurrentSettings.Trigger1Temp = 60;
  CurrentSettings.Trigger2Type = false;
  CurrentSettings.Trigger2Mode = true;
  CurrentSettings.Trigger2OnDelay = 10;
  CurrentSettings.Trigger2InactTimer = 0;
  CurrentSettings.Trigger2Temp = 60;
  CurrentSettings.ScreenSaverActive = true;
  CurrentSettings.DisplayOnLevel = 0xFF;
  CurrentSettings.DisplayDimLevel = 0x00;
  CurrentSettings.DisplayTimeout = 30;
  CurrentSettings.DisplaySelectedInput = true;
  CurrentSettings.DisplayTemperature1 = 3;
  CurrentSettings.DisplayTemperature2 = 3;
  CurrentSettings.Version = VERSION;
}

// Write CurrentSettings to EEPROM
void writeSettingsToEEPROM()
{
  // Write the settings to the EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.write(0, CurrentSettings.data, sizeof(CurrentSettings));
}

// Write Default Settings to EEPROM
void writeDefaultSettingsToEEPROM()
{
  // Read default settings into CurrentSettings
  setCurrentSettingsToDefault();
  // Write the settings to the EEPROM
  writeSettingsToEEPROM();
}

// Read CurrentSettings from EEPROM
void readSettingsFromEEPROM()
{
  // Read settings from EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.read(0, CurrentSettings.data, sizeof(CurrentSettings));
}

// Reboots the sketch - used after restoring default settings
void reboot()
{
  // TO DO Unselect all inputs (unless all inputs are deactivated by the MCP23008 during reboot - has to be tested)
  // TO DO Mute volume control)
  lcd.clear();
  lcd.setCursor(0, 2);
  lcd.print("REBOOTING...");
  delay(2000);
  lcd.clear();
  asm volatile("  jmp 0"); // Restarts the sketch
}
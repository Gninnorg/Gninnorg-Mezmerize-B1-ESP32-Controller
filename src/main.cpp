/*
**
**    Controller for Mezmerize B1 Buffer using Muses72320 potentiometer 
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

unsigned long mil_On = millis(); // Holds the millis from last power on (or reboot)

struct InputSettings
{
  byte Active;
  char Name[11];
  byte MaxVol;
  byte MinVol;
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
    byte RecallSetLevel;           // Remember/store the volume level for each separate input
    HashIR_data_t IR_ONOFF;        // IR data to be interpreted as ON/OFF - switch between running and suspend mode (and turn triggers off)
    HashIR_data_t IR_UP;           // IR data to be interpreted as UP
    HashIR_data_t IR_DOWN;         // IR data to be interpreted as DOWN
    HashIR_data_t IR_REPEAT;       // IR data to be interpreted as REPEAT (ie Apple remotes sends a specific code, if a key is held down to indicate repeat of the previously sent code
    HashIR_data_t IR_LEFT;         // IR data to be interpreted as LEFT
    HashIR_data_t IR_RIGHT;        // IR data to be interpreted as RIGHT
    HashIR_data_t IR_SELECT;       // IR data to be interpreted as SELECT
    HashIR_data_t IR_BACK;         // IR data to be interpreted as BACK
    HashIR_data_t IR_MUTE;         // IR data to be interpreted as MUTE
    HashIR_data_t IR_PREVIOUS;     // IR data to be interpreted as "switch to previous selected input"
    HashIR_data_t IR_1;            // IR data to be interpreted as 1 (to select input 1 directly)
    HashIR_data_t IR_2;            // IR data to be interpreted as 2
    HashIR_data_t IR_3;            // IR data to be interpreted as 3
    HashIR_data_t IR_4;            // IR data to be interpreted as 4
    HashIR_data_t IR_5;            // IR data to be interpreted as 5
    HashIR_data_t IR_6;            // IR data to be interpreted as 6
    struct InputSettings Input[6]; // Settings for all 6 inputs
    byte Trigger1Active;           // 0 = the trigger is not active, 1 = the trigger is active
    byte Trigger1Type;             // 0 = momentary, 1 = latching
    byte Trigger1Mode;             // 0 = standard, 1 = intelligent (with measurement of NTC+LDR value)
    byte Trigger1OnDelay;          // Seconds from controller power up to activation of trigger. The default delay allows time for the output relay of the Mezmerize to be activated before we turn on the power amps. The selection of an input of the Mezmerize will also be delayed.
    byte Trigger1Temp;             // Temperature protection: if the temperature is measured to the set number of degrees Celcius (via the LDRs), the controller will attempt to trigger a shutdown of the connected power amps (if set to 0, the temperature protection is not active
    byte Trigger2Active;           // 0 = the trigger is not active, 1 = the trigger is active
    byte Trigger2Type;             // 0 = momentary, 1 = latching
    byte Trigger2Mode;             // 0 = standard, 1 = intelligent (with measurement of NTC+LDR value)
    byte Trigger2OnDelay;          // Seconds from controller power up to activation of trigger. The default delay allows time for the output relay of the Mezmerize to be activated before we turn on the power amps. The selection of an input of the Mezmerize will also be delayed.
    byte Trigger2Temp;             // Temperature protection: if the temperature is measured to the set number of degrees Celcius (via the LDRs), the controller will attempt to trigger a shutdown of the connected power amps (if set to 0, the temperature protection is not active)
    byte TriggerInactOffTimer;     // Hours without user interaction before automatic power down (0 = never)
    byte ScreenSaverActive;        // 0 = the display will stay on/not be dimmed, 1 = the display will be dimmed to the specified level after a specified period of time with no user input
    byte DisplayOnLevel;           // The contrast level of the display when it is on, 0 = 25%, 1 = 50%, 2 = 75%, 3 = 100%
    byte DisplayDimLevel;          // The contrast level of the display when screen saver is active. 0 = off, 1 = 3, 2 = 7 ... 32 = 127. If DisplayDimLevel = 0 the display will be turned off when the screen saver is active (to reduce electrical noise)
    byte DisplayTimeout;           // Number of seconds before the screen saver is activated.
    byte DisplaySelectedInput;     // 0 = the name of the active input is not shown on the display (ie. if only one input is used), 1 = the name of the selected input is shown on the display
    byte DisplayTemperature1;      // 0 = do not display the temperature measured by NTC 1, 1 = display in number of degrees Celcious, 2 = display as graphical representation, 3 = display both
    byte DisplayTemperature2;      // 0 = do not display the temperature measured by NTC 2, 1 = display in number of degrees Celcious, 2 = display as graphical representation, 3 = display both
    float Version;                 // Used to check if data read from the EEPROM is valid with the compiled version of the compiled code - if not a reset to defaults is necessary and they must be written to the EEPROM
  };
  byte data[]; // Allows us to be able to write/read settings from EEPROM byte-by-byte (to avoid specific serialization/deserialization code)
} Settings;

Settings CurrentSettings; // Holds all the current settings
void setCurrentSettingsToDefault(void);

typedef union {
  struct
  {
    byte CurrentInput;      // The number of the currently set input
    byte CurrentVolume;     // The currently set volume
    bool Muted;             // Indicates if we are in mute mode or not
    byte InputLastVol[6];   // The last volume set for each input
    byte PrevSelectedInput; // Holds the input selected before the current one
    float Version;          // Used to check if data read from the EEPROM is valid with the compiled version of the compiled code - if not a reset to defaults is necessary and they must be written to the EEPROM
  };
  byte data[]; // Allows us to be able to write/read settings from EEPROM byte-by-byte (to avoid specific serialization/deserialization code)
} RuntimeSettings;

RuntimeSettings CurrentRuntimeSettings;

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
void readRuntimeSettingsFromEEPROM(void);
void writeRuntimeSettingsToEEPROM(void);

// Setup Display ---------------------------------------------------------------
OLedI2C lcd;
// Used to indicate whether the screen saver is running or not
bool ScreenSaverIsOn = false;
// Used to keep track of the time of the last user interaction (part of the screen saver timing)
unsigned long mil_LastUserInput = millis();
// Used to time how often the display of temperatures is updated
unsigned long mil_onRefreshTemperatureDisplay;
// Update interval for the display of temperatures
#define TEMP_REFRESH_INTERVAL 5000

void setupDisplay()
{
  lcd.begin();
  lcd.backlight((CurrentSettings.DisplayOnLevel + 1) * 64 - 1);
  lcd.clear();
  lcd.defineCustomChar();
}

//  Initialize the menu
enum AppModeValues
{
  APP_NORMAL_MODE,
  APP_MENU_MODE,
  APP_PROCESS_MENU_CMD,
  APP_STANDBY_MODE,
  APP_POWERLOSS_STATE
};

byte appMode = APP_NORMAL_MODE;

MenuManager Menu1(ctlMenu_Root, menuCount(ctlMenu_Root));

void editInputName(uint8_t InputNumber);
void drawEditInputNameScreen(bool isUpperCase);
bool editNumericValue(byte &Value, byte MinValue, byte MaxValue);
bool editOptionValue(byte &Value, byte NumOptions, const char Option1[9], const char Option2[9], const char Option3[9], const char Option4[9]);
bool editIRCode(HashIR_data_t &Value);
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
  KEY_NONE,    // No input
  KEY_UP,      // Rotary 1 turned CW or IR
  KEY_DOWN,    // Rotary 1 turned CCW or IR
  KEY_REPEAT,  // IR
  KEY_SELECT,  // Rotary 1 switch pressed or IR
  KEY_RIGHT,   // Rotary 2 turned CW or IR
  KEY_LEFT,    // Rotary 2 turned CCW or IR
  KEY_BACK,    // Rotary 2 switch pressed or IR
  KEY_1,       // IR
  KEY_2,       // IR
  KEY_3,       // IR
  KEY_4,       // IR
  KEY_5,       // IR
  KEY_6,       // IR
  KEY_MUTE,    // IR
  KEY_ONOFF,   // IR
  KEY_PREVIOUS // IR
};

byte UIkey; // holds the last received user input (from rotary encoders or IR)
byte lastReceivedInput = KEY_NONE;
unsigned long last_KEY_ONOFF = millis(); // Used to ensure that fast repetition of KEY_ONOFF is not accepted
void toStandbyMode(void);

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
  case ClickEncoder::Clicked:
    receivedInput = KEY_BACK;
    break;
  case ClickEncoder::DoubleClicked:
    receivedInput = KEY_ONOFF;
    break;
  default:
    break;
  }

  // Check if any input from the IR remote
  if (IRLremote.available())
  {
    // Get the new data from the remote
    auto data = IRLremote.read();

    //Serial.print(F("Address: 0x"));
    //Serial.println(data.address, HEX);
    //Serial.print(F("Command: 0x"));
    //Serial.println(data.command, HEX);

    // Map the received IR input to UserInput values
    if (data.address == CurrentSettings.IR_UP.address && data.command == CurrentSettings.IR_UP.command)
      receivedInput = KEY_UP;
    else if (data.address == CurrentSettings.IR_DOWN.address && data.command == CurrentSettings.IR_DOWN.command)
      receivedInput = KEY_DOWN;
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
    else if (data.address == CurrentSettings.IR_REPEAT.address && data.command == CurrentSettings.IR_REPEAT.command)
    {
      receivedInput = KEY_REPEAT;
      if (lastReceivedInput == KEY_UP)
        receivedInput = KEY_UP;
      else if (lastReceivedInput == KEY_DOWN)
        receivedInput = KEY_DOWN;
    }
    lastReceivedInput = receivedInput;
  }

  // Cancel received KEY_ONOFF if it has been received before within the last 5 seconds
  if (receivedInput == KEY_ONOFF)
  {
    if (last_KEY_ONOFF + 5000 > millis())
      receivedInput = KEY_NONE;
    else
    {
      last_KEY_ONOFF = millis();
      if (appMode != APP_STANDBY_MODE)
      {
        appMode = APP_STANDBY_MODE;
        toStandbyMode();
      }
      else                       // wake from standby - we do it by restarting the sketch (a bit hardcore, but it works ;-)
        asm volatile("  jmp 0"); // Restarts the sketch
    }
  }

  // Turn Screen Saver on/off if it is activated and if no user input has been received during the defined number of seconds
  if (receivedInput == KEY_NONE && CurrentSettings.ScreenSaverActive && appMode != APP_STANDBY_MODE)
  {
    if (!ScreenSaverIsOn && (millis() - mil_LastUserInput > (unsigned long)CurrentSettings.DisplayTimeout * 1000))
    {
      if (CurrentSettings.DisplayDimLevel == 0)
        lcd.lcdOff();
      else
        lcd.backlight(CurrentSettings.DisplayDimLevel * 4 - 1);
      ScreenSaverIsOn = true;
    }
  }
  else
  {
    mil_LastUserInput = millis();
    if (ScreenSaverIsOn)
    {
      if (CurrentSettings.DisplayDimLevel == 0)
        lcd.lcdOn();
      else
        lcd.backlight((CurrentSettings.DisplayOnLevel + 1) * 64 - 1);
      ScreenSaverIsOn = false;
    }
  }

  // If inactivity timer is set, go to standby if the set number of hours have passed since last user input
  if ((appMode != APP_STANDBY_MODE) && (CurrentSettings.TriggerInactOffTimer > 0) && ((mil_LastUserInput + CurrentSettings.TriggerInactOffTimer * 3600000) < millis()))
  {
    appMode = APP_STANDBY_MODE;
    toStandbyMode();
  }

  return (receivedInput);
}

void reboot(void);
void DisplayTemperatures(void);

void setup()
{
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  Serial.begin(115200);
  Wire.begin();
  setupRotaryEncoders();
  setupIR();
  setupDisplay();
  setupRelayController();
  readSettingsFromEEPROM();
  readRuntimeSettingsFromEEPROM();

  // Check if settings stored in EEPROM are NOT valid - we write the default settings to the EEPROM and reboots
  if ((CurrentSettings.Version != VERSION) || (CurrentRuntimeSettings.Version != VERSION))
  {
    //Serial.println(F("ERROR: Settings read from EEPROM are not ok"));
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
  else // Settings read from EEPROM are valid so let's move on!
  {
    // TO DO if triggers are active then wait for the set number of seconds (if > 0) and turn them on with the chosen method
    // TO DO Select input relay
    // TO DO Set volume
    lcd.printTwoNumber(11, CurrentRuntimeSettings.CurrentVolume);
    if (CurrentSettings.DisplaySelectedInput)
    {
      lcd.setCursor(0, 0);
      lcd.print(CurrentSettings.Input[CurrentRuntimeSettings.CurrentInput].Name);
    }
    DisplayTemperatures();
  }
}

void DisplayTemperatures()
{
  if (CurrentSettings.DisplayTemperature1)
  {
    float Temp = relayControl.getTemperature(A0);
    float MaxTemp;
    if (CurrentSettings.Trigger1Temp == 0)
      MaxTemp = 60;
    else
      MaxTemp = CurrentSettings.Trigger1Temp;
    lcd.setCursor(0, 3);
    if (Temp < 0)
    {
      lcd.print("OFF ");
      if (CurrentSettings.DisplayTemperature1 == 3)
      {
        lcd.setCursor(0, 2);
        lcd.print("AMP ");
      }
    }
    else if (Temp > MaxTemp)
    {
      lcd.setCursor(0, 3);
      lcd.print("HIGH");
      if (CurrentSettings.DisplayTemperature1 == 3)
      {
        lcd.setCursor(0, 2);
        lcd.print("TEMP");
      }
    }
    else
    {
      if (CurrentSettings.DisplayTemperature1 == 1 || CurrentSettings.DisplayTemperature1 == 3)
      {
        lcd.setCursor(0, 3);
        lcd.print(int(Temp));
        lcd.write(128); // Degree symbol
        lcd.print(" ");
      }
      if (CurrentSettings.DisplayTemperature1 == 2 || CurrentSettings.DisplayTemperature1 == 3)
      {
        if (CurrentSettings.DisplayTemperature1 == 2)
          lcd.setCursor(0, 3);
        else
          lcd.setCursor(0, 2);

        // Map the range (0c ~ max temperature) to the range of the bar (0 to Number of characters to show the bar * Number of possible values per character )
        byte nb_columns = map(Temp, 0, MaxTemp, 0, 4 * 5);

        for (byte i = 0; i < 4; ++i) // Number of characters to show the bar = 4
        {

          // Write character depending on number of columns remaining to display
          if (nb_columns == 0)
          { // Case empty
            lcd.write(' ');
          }
          else if (nb_columns >= 5) // Full box
          {
            lcd.write(208); // Full box symbol
            nb_columns -= 5;
          }
          else // Partial box
          {
            lcd.write(map(nb_columns, 1, 4, 212, 209)); // Map the remaining nb_columns (case between 1 and 4) to the corresponding character number symbols : 212 = 1 bar, 211 = 2 bars, 210 = 3 bars, 209 = 4 bars
            nb_columns = 0;
          }
        }
      }
    }
  }

  if (CurrentSettings.DisplayTemperature2)
  {
    float Temp = relayControl.getTemperature(A1);
    float MaxTemp;
    if (CurrentSettings.Trigger2Temp == 0)
      MaxTemp = 60;
    else
      MaxTemp = CurrentSettings.Trigger2Temp;
    byte Col;
    if (CurrentSettings.DisplayTemperature1)
      Col = 5;
    else
      Col = 0;
    lcd.setCursor(Col, 3);
    if (Temp < 0)
    {
      lcd.print("OFF ");
      if (CurrentSettings.DisplayTemperature2 == 3)
      {
        lcd.setCursor(Col, 2);
        lcd.print("AMP ");
      }
    }
    else if (Temp > MaxTemp)
    {
      lcd.setCursor(Col, 3);
      lcd.print("HIGH");
      if (CurrentSettings.DisplayTemperature2 == 3)
      {
        lcd.setCursor(Col, 2);
        lcd.print("TEMP");
      }
    }
    else
    {
      if (CurrentSettings.DisplayTemperature2 == 1 || CurrentSettings.DisplayTemperature2 == 3)
      {
        lcd.setCursor(Col, 3);
        lcd.print(int(Temp));
        lcd.write(128); // Degree symbol
        lcd.print(" ");
      }
      if (CurrentSettings.DisplayTemperature2 == 2 || CurrentSettings.DisplayTemperature2 == 3)
      {
        if (CurrentSettings.DisplayTemperature2 == 2)
          lcd.setCursor(Col, 3);
        else
          lcd.setCursor(Col, 2);

        // Map the range (0c ~ max temperature) to the range of the bar (0 to Number of characters to show the bar * Number of possible values per character )
        byte nb_columns = map(Temp, 0, MaxTemp, 0, 4 * 5);

        for (byte i = 0; i < 4; ++i) // Number of characters to show the bar = 4
        {

          // Write character depending on number of columns remaining to display
          if (nb_columns == 0)
          { // Case empty
            lcd.write(' ');
          }
          else if (nb_columns >= 5) // Full box
          {
            lcd.write(208); // Full box symbol
            nb_columns -= 5;
          }
          else // Partial box
          {
            lcd.write(map(nb_columns, 1, 4, 212, 209)); // Map the remaining nb_columns (case between 1 and 4) to the corresponding character number symbols : 212 = 1 bar, 211 = 2 bars, 210 = 3 bars, 209 = 4 bars
            nb_columns = 0;
          }
        }
      }
    }
  }

  mil_onRefreshTemperatureDisplay = millis();
}

void loop()
{
  UIkey = getUserInput();

  // Detect power off
  // If low power is detected the RuntimeSettings are written to EEPROM. We only write these data when power down is detected to avoid to write to the EEPROM every time the volume or input is changed (an EEPROM has a limited lifetime of about 100000 write cycles)
  // TO DO This could be moved to getUserInput() to ensure it is always checked - maybe also set appMode to APP_STANDBY_MODE to ensure the possibility to wake up again with ONOFF if the power drop is only very brief (and not a total loss of power)
  // TO DO The downside to checking this in getUserInput() is that it slows down but maybe it doesn't matter in practice.
  if (appMode != APP_POWERLOSS_STATE)
  {
    long vcc;
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2);
    ADCSRA |= _BV(ADSC);
    while (bit_is_set(ADCSRA, ADSC))
      ;
    vcc = ADCL;
    vcc |= ADCH << 8;
    vcc = 1126400L / vcc;
    if (vcc > 3000 && vcc < 4600)
    {
      writeRuntimeSettingsToEEPROM();
      // TO DO Turn off trigger relays
      appMode = APP_POWERLOSS_STATE; // Switch to APP_STATE_OFF and do nothing until power disappears completely
    }
  }

  switch (appMode)
  {
  case APP_NORMAL_MODE:
    if (millis() > mil_onRefreshTemperatureDisplay + TEMP_REFRESH_INTERVAL)
      DisplayTemperatures();

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
      if (!CurrentRuntimeSettings.Muted && (CurrentRuntimeSettings.CurrentVolume < CurrentSettings.MaxVolume) && (CurrentRuntimeSettings.CurrentVolume < CurrentSettings.Input[CurrentRuntimeSettings.CurrentInput].MaxVol))
      {
        CurrentRuntimeSettings.CurrentVolume++;
        CurrentRuntimeSettings.InputLastVol[CurrentRuntimeSettings.CurrentInput] = CurrentRuntimeSettings.CurrentVolume;
        lcd.printTwoNumber(11, CurrentRuntimeSettings.CurrentVolume);
        // TO DO Set volume to CurrentVolume
        // TO DO Save CurrentVolume to EEPROM
      }
      break;
    case KEY_DOWN:
      // Turn volume down if we're not muted and we'll not get below the global minimum volume or the minimum volume set for the currently selected input
      if (!CurrentRuntimeSettings.Muted && (CurrentRuntimeSettings.CurrentVolume > CurrentSettings.MinVolume) && (CurrentRuntimeSettings.CurrentVolume > CurrentSettings.Input[CurrentRuntimeSettings.CurrentInput].MinVol))
      {
        CurrentRuntimeSettings.CurrentVolume--;
        CurrentRuntimeSettings.InputLastVol[CurrentRuntimeSettings.CurrentInput] = CurrentRuntimeSettings.CurrentVolume;
        lcd.printTwoNumber(11, CurrentRuntimeSettings.CurrentVolume);
        // TO DO Set volume to CurrentVolume
        // TO DO Save CurrentVolume to EEPROM
      }
      break;
    case KEY_LEFT:
    case KEY_RIGHT:
    {
      byte nextInput;
      if (UIkey == KEY_RIGHT) // Switch to next active input with an input number larger than the current one
      {
        if (CurrentRuntimeSettings.CurrentInput == 5)
          nextInput = 0;
        else
          nextInput = CurrentRuntimeSettings.CurrentInput + 1;
        while (!CurrentSettings.Input[nextInput].Active)
        {
          nextInput++;
          if (nextInput > 5)
            nextInput = 0;
        }
      }
      else // Switch to next active input with an input number less than the current one
      {
        if (CurrentRuntimeSettings.CurrentInput == 0)
          nextInput = 5;
        else
          nextInput = CurrentRuntimeSettings.CurrentInput - 1;
        while (!CurrentSettings.Input[nextInput].Active)
        {
          if (CurrentRuntimeSettings.CurrentInput == 0)
            nextInput = 5;
          else
            nextInput--;
        }
      }

      if (CurrentRuntimeSettings.CurrentInput != nextInput) // Change settings if it was possible to change to another input number
      {
        CurrentRuntimeSettings.PrevSelectedInput = CurrentRuntimeSettings.CurrentInput; // Save the current input as the previous selected input
        CurrentRuntimeSettings.CurrentInput = nextInput;
        // TO DO Mute
        // TO DO switch to nextInput
        if (CurrentSettings.RecallSetLevel)
          CurrentRuntimeSettings.CurrentVolume = CurrentRuntimeSettings.InputLastVol[CurrentRuntimeSettings.CurrentInput];

        // TO DO set volume to CurrentSettings.CurrentVolume (remember validations against global volume levels and local volume levels)
        lcd.printTwoNumber(11, CurrentRuntimeSettings.CurrentVolume);
        // TO DO Unmute
        if (CurrentSettings.DisplaySelectedInput)
        {
          lcd.setCursor(0, 0);
          lcd.print(CurrentSettings.Input[CurrentRuntimeSettings.CurrentInput].Name);
        }
      }
    }
    break;
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    {
      byte inputNumber;
      // TO DO The switch below can be replaced by inputNumber = UIkey - KEY_1 but it is not quite as readable?
      switch (UIkey)
      {
      case KEY_1:
        inputNumber = 0;
        break;
      case KEY_2:
        inputNumber = 1;
        break;
      case KEY_3:
        inputNumber = 2;
        break;
      case KEY_4:
        inputNumber = 3;
        break;
      case KEY_5:
        inputNumber = 4;
        break;
      case KEY_6:
        inputNumber = 5;
        break;
      }
      // Select input (if active)
      if (CurrentSettings.Input[inputNumber].Active)
      {
        if (CurrentRuntimeSettings.CurrentInput != inputNumber) // Change settings
        {
          CurrentRuntimeSettings.PrevSelectedInput = CurrentRuntimeSettings.CurrentInput; // Save the current input as the previous selected input
          CurrentRuntimeSettings.CurrentInput = inputNumber;
          // TO DO Mute
          // TO DO switch to nextInput
          if (CurrentSettings.RecallSetLevel)
            CurrentRuntimeSettings.CurrentVolume = CurrentRuntimeSettings.InputLastVol[CurrentRuntimeSettings.CurrentInput];

          // TO DO set volume to CurrentSettings.CurrentVolume (remember validations against global volume levels and local volume levels)
          lcd.printTwoNumber(11, CurrentRuntimeSettings.CurrentVolume);
          // TO DO Unmute
          if (CurrentSettings.DisplaySelectedInput)
          {
            lcd.setCursor(0, 0);
            lcd.print(CurrentSettings.Input[CurrentRuntimeSettings.CurrentInput].Name);
          }
        }
      }
    }
    break;
    case KEY_PREVIOUS:
      // TO DO Switch to previous selected input
      break;
    case KEY_MUTE:
      // toggle mute
      if (CurrentRuntimeSettings.Muted)
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

  case APP_MENU_MODE:
  { // Brackets to avoid warning: "jump to case label [-fpermissive]"
    byte menuMode = Menu1.handleNavigation(getNavAction, refreshMenuDisplay);

    if (menuMode == MENU_EXIT)
    {
      lcd.clear();
      // Back to APP_NORMAL_MODE
      lcd.printTwoNumber(11, CurrentRuntimeSettings.CurrentVolume);
      lcd.setCursor(0, 0);
      if (CurrentSettings.DisplaySelectedInput)
        lcd.print(CurrentSettings.Input[CurrentRuntimeSettings.CurrentInput].Name);
      DisplayTemperatures();

      appMode = APP_NORMAL_MODE;
    }
    else if (menuMode == MENU_INVOKE_ITEM) // TO DO MENU_INVOKE_ITEM seems to be superfluous after my other changes
    {
      appMode = APP_PROCESS_MENU_CMD;
    }
    break;
  }
  case APP_PROCESS_MENU_CMD:
  {
    byte processingComplete = processMenuCommand(Menu1.getCurrentItemCmdId());

    if (processingComplete)
    {
      appMode = APP_MENU_MODE;
      drawMenu();
    }
    break;
  }
  case APP_STANDBY_MODE:
    // Do nothing if in APP_STANDBY_MODE - if the user presses KEY_ONOFF a restart/reboot is done by getUserInput(). By the way: you don't need an IR remote to be set up - a doubleclick on encoder_2 is also KEY_ONOFF
    break;
  case APP_POWERLOSS_STATE: // Only active if power drop is detected
    Serial.println("In APP_POWERLOSS_STATE");
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("ATTENTION:");
    lcd.setCursor(0,2);
    lcd.print("Check power supply!");
    delay(2000);
    lcd.clear();
    long vcc;
    do 
    {
      ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
      delay(2);
      ADCSRA |= _BV(ADSC);
      while (bit_is_set(ADCSRA, ADSC))
        ;
      vcc = ADCL;
      vcc |= ADCH << 8;
      vcc = 1126400L / vcc;
      Serial.print("Voltage: ");
      Serial.println(vcc);
    } while (vcc < 4700); // Wait until power is completely gone or reboot if it returns
    reboot();
    break;
  }
}

void toStandbyMode()
{
  writeRuntimeSettingsToEEPROM();
  if (ScreenSaverIsOn)
  {
    if (CurrentSettings.DisplayDimLevel == 0)
      lcd.lcdOn();
    else
      lcd.backlight((CurrentSettings.DisplayOnLevel + 1) * 64 - 1);
    ScreenSaverIsOn = false;
  }
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(F("Going to sleep!"));
  lcd.setCursor(0, 3);
  lcd.print(F("           ...zzzZZZ"));
  // TO DO Mute output
  // TO DO Turn of triggers
  delay(2000);
  lcd.PowerDown();
  while (getUserInput() != KEY_ONOFF)
    ;
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
    editNumericValue(CurrentSettings.VolumeSteps, 0, 99);
    // TO DO Validate if VolumeSteps < Max_Volume (both global and for each input) - if so, they must be changed. Also if Max vol's are changed then maybe min. vol's needs to be changed also. Same goes for max start vol and mute lvl
    complete = true;
    break;
  }
  case mnuCmdMIN_VOL:
    editNumericValue(CurrentSettings.MinVolume, 0, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdMAX_VOL:
    editNumericValue(CurrentSettings.MaxVolume, CurrentSettings.MinVolume, CurrentSettings.VolumeSteps);
    complete = true;
    break;
  case mnuCmdMAX_START_VOL:
    editNumericValue(CurrentSettings.MaxStartVolume, CurrentSettings.MinVolume, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdMUTE_LVL:
    editNumericValue(CurrentSettings.MuteLevel, 0, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdSTORE_LVL:
    editOptionValue(CurrentSettings.RecallSetLevel, 2, "No", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT1_ACTIVE:
    editOptionValue(CurrentSettings.Input[0].Active, 2, "No", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT1_NAME:
    editInputName(0);
    complete = true;
    break;
  case mnuCmdINPUT1_MAX_VOL:
    editNumericValue(CurrentSettings.Input[0].MaxVol, CurrentSettings.MinVolume, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdINPUT1_MIN_VOL:
    editNumericValue(CurrentSettings.Input[0].MinVol, CurrentSettings.MinVolume, CurrentSettings.Input[0].MaxVol);
    complete = true;
    break;
  case mnuCmdINPUT2_ACTIVE:
    editOptionValue(CurrentSettings.Input[1].Active, 2, "No", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT2_NAME:
    editInputName(1);
    complete = true;
    break;
  case mnuCmdINPUT2_MAX_VOL:
    editNumericValue(CurrentSettings.Input[1].MaxVol, CurrentSettings.MinVolume, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdINPUT2_MIN_VOL:
    editNumericValue(CurrentSettings.Input[1].MinVol, CurrentSettings.MinVolume, CurrentSettings.Input[1].MaxVol);
    complete = true;
    break;
  case mnuCmdINPUT3_ACTIVE:
    editOptionValue(CurrentSettings.Input[2].Active, 2, "No", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT3_NAME:
    editInputName(2);
    complete = true;
    break;
  case mnuCmdINPUT3_MAX_VOL:
    editNumericValue(CurrentSettings.Input[2].MaxVol, CurrentSettings.MinVolume, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdINPUT3_MIN_VOL:
    editNumericValue(CurrentSettings.Input[2].MinVol, CurrentSettings.MinVolume, CurrentSettings.Input[2].MaxVol);
    complete = true;
    break;
  case mnuCmdINPUT4_ACTIVE:
    editOptionValue(CurrentSettings.Input[3].Active, 2, "No", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT4_NAME:
    editInputName(3);
    complete = true;
    break;
  case mnuCmdINPUT4_MAX_VOL:
    editNumericValue(CurrentSettings.Input[3].MaxVol, CurrentSettings.MinVolume, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdINPUT4_MIN_VOL:
    editNumericValue(CurrentSettings.Input[3].MinVol, CurrentSettings.MinVolume, CurrentSettings.Input[3].MaxVol);
    complete = true;
    break;
  case mnuCmdINPUT5_ACTIVE:
    editOptionValue(CurrentSettings.Input[4].Active, 2, "No", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT5_NAME:
    editInputName(4);
    complete = true;
    break;
  case mnuCmdINPUT5_MAX_VOL:
    editNumericValue(CurrentSettings.Input[4].MaxVol, CurrentSettings.MinVolume, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdINPUT5_MIN_VOL:
    editNumericValue(CurrentSettings.Input[4].MinVol, CurrentSettings.MinVolume, CurrentSettings.Input[4].MaxVol);
    complete = true;
    break;
  case mnuCmdINPUT6_ACTIVE:
    editOptionValue(CurrentSettings.Input[5].Active, 2, "No", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT6_NAME:
    editInputName(5);
    complete = true;
    break;
  case mnuCmdINPUT6_MAX_VOL:
    editNumericValue(CurrentSettings.Input[5].MaxVol, CurrentSettings.MinVolume, CurrentSettings.MaxVolume);
    complete = true;
    break;
  case mnuCmdINPUT6_MIN_VOL:
    editNumericValue(CurrentSettings.Input[5].MinVol, CurrentSettings.MinVolume, CurrentSettings.Input[5].MaxVol);
    complete = true;
    break;
  case mnuCmdIR_ONOFF:
    editIRCode(CurrentSettings.IR_ONOFF);
    complete = true;
    break;
  case mnuCmdIR_UP:
    editIRCode(CurrentSettings.IR_UP);
    complete = true;
    break;
  case mnuCmdIR_DOWN:
    editIRCode(CurrentSettings.IR_DOWN);
    complete = true;
    break;
  case mnuCmdIR_REPEAT:
    editIRCode(CurrentSettings.IR_REPEAT);
    complete = true;
    break;
  case mnuCmdIR_LEFT:
    editIRCode(CurrentSettings.IR_LEFT);
    complete = true;
    break;
  case mnuCmdIR_RIGHT:
    editIRCode(CurrentSettings.IR_RIGHT);
    complete = true;
    break;
  case mnuCmdIR_SELECT:
    editIRCode(CurrentSettings.IR_SELECT);
    complete = true;
    break;
  case mnuCmdIR_BACK:
    editIRCode(CurrentSettings.IR_BACK);
    complete = true;
    break;
  case mnuCmdIR_MUTE:
    editIRCode(CurrentSettings.IR_MUTE);
    complete = true;
    break;
  case mnuCmdIR_PREV:
    editIRCode(CurrentSettings.IR_PREVIOUS);
    complete = true;
    break;
  case mnuCmdIR_1:
    editIRCode(CurrentSettings.IR_1);
    complete = true;
    break;
  case mnuCmdIR_2:
    editIRCode(CurrentSettings.IR_2);
    complete = true;
    break;
  case mnuCmdIR_3:
    editIRCode(CurrentSettings.IR_3);
    complete = true;
    break;
  case mnuCmdIR_4:
    editIRCode(CurrentSettings.IR_4);
    complete = true;
    break;
  case mnuCmdIR_5:
    editIRCode(CurrentSettings.IR_5);
    complete = true;
    break;
  case mnuCmdIR_6:
    editIRCode(CurrentSettings.IR_6);
    complete = true;
    break;
  case mnuCmdTRIGGER1_ACTIVE:
    editOptionValue(CurrentSettings.Trigger1Active, 2, "Inactive", "Active", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER1_TYPE:
    editOptionValue(CurrentSettings.Trigger1Mode, 2, "Moment.", "Latching", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER1_MODE:
    editOptionValue(CurrentSettings.Trigger1Type, 2, "Standard", "SmartON", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER1_ON_DELAY:
    editNumericValue(CurrentSettings.Trigger1OnDelay, 0, 90);
    complete = true;
    break;
  case mnuCmdTRIGGER1_TEMP:
    editNumericValue(CurrentSettings.Trigger1Temp, 0, 90);
    complete = true;
    break;
  case mnuCmdTRIGGER2_ACTIVE:
    editOptionValue(CurrentSettings.Trigger2Active, 2, "Inactive", "Active", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER2_TYPE:
    editOptionValue(CurrentSettings.Trigger2Mode, 2, "Moment.", "Latching", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER2_MODE:
    editOptionValue(CurrentSettings.Trigger2Type, 2, "Standard", "SmartON", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER2_ON_DELAY:
    editNumericValue(CurrentSettings.Trigger2OnDelay, 0, 90);
    complete = true;
    break;
  case mnuCmdTRIGGER2_TEMP:
    editNumericValue(CurrentSettings.Trigger2Temp, 0, 90);
    complete = true;
    break;
  case mnuCmdTRIGGER_INACT_TIMER:
    editNumericValue(CurrentSettings.TriggerInactOffTimer, 0, 24);
    complete = true;
    break;
  case mnuCmdDISP_SAVER_ACTIVE:
    editOptionValue(CurrentSettings.ScreenSaverActive, 2, "Off", "On", "", "");
    complete = true;
    break;
  case mnuCmdDISP_ON_LEVEL:
    editOptionValue(CurrentSettings.DisplayOnLevel, 4, "25%", "50%", "75%", "100%");
    lcd.backlight((CurrentSettings.DisplayOnLevel + 1) * 64 - 1);
    complete = true;
    break;
  case mnuCmdDISP_DIM_LEVEL:
    editNumericValue(CurrentSettings.DisplayDimLevel, 0, 32);
    if (CurrentSettings.DisplayDimLevel != 0)
    {
      lcd.backlight(CurrentSettings.DisplayDimLevel * 4 - 1);
      delay(2000);
      lcd.backlight((CurrentSettings.DisplayOnLevel + 1) * 64 - 1);
    }
    complete = true;
    break;
  case mnuCmdDISP_DIM_TIMEOUT:
    editNumericValue(CurrentSettings.DisplayTimeout, 0, 90);
    complete = true;
    break;
  case mnuCmdDISP_INPUT:
    editOptionValue(CurrentSettings.DisplaySelectedInput, 2, "Hide", "Show", "", "");
    complete = true;
    break;
  case mnuCmdDISP_TEMP1:
    editOptionValue(CurrentSettings.DisplayTemperature1, 4, "None", "Degrees", "Bar", "Both");
    complete = true;
    break;
  case mnuCmdDISP_TEMP2:
    editOptionValue(CurrentSettings.DisplayTemperature2, 4, "None", "Degrees", "Bar", "Both");
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

void editInputName(uint8_t InputNumber)
{
  bool complete = false;
  bool isUpperCase = true;
  // TO DO Maybe replace / character with symbol to allow for switching between upper and lower case letters?
  int arrowX = 1;              // text edit arrow start X position on selection line
  int arrowPointingUpDown = 0; // text edit arrow start direction: up == 0, down == 1
  String newInputName = "XXXXXXXXXX";
  // Display the screen
  lcd.clear();
  lcd.print("Input ");
  lcd.print(InputNumber + 1);
  lcd.setCursor(7, 0);
  lcd.write(223); // Right arrow
  drawEditInputNameScreen(isUpperCase);

  lcd.setCursor(arrowX, 2);
  lcd.write(byte(26 + arrowPointingUpDown)); // 26 is arrow up, 27 is arrow down
  newInputName = CurrentSettings.Input[InputNumber].Name;
  newInputName.trim();
  lcd.setCursor(9, 0);
  lcd.print(newInputName);
  lcd.setCursor(9 + newInputName.length(), 0);
  lcd.BlinkingCursorOn();
  while (!complete)
  {
    mil_LastUserInput = millis(); // Prevent the screen saver to kick in while editing
    switch (byte UserInput = getUserInput())
    {
    case KEY_RIGHT:
    case KEY_LEFT:
      lcd.BlinkingCursorOff();
      // Clear current arrow
      lcd.setCursor(arrowX, 2);
      lcd.write(' ');

      // Decide if position or direction of arrow must be changed
      if (arrowPointingUpDown == 0 && UserInput == KEY_RIGHT)     // The arrow points up and the user input is "turn to the right"
        arrowPointingUpDown = 1;                                  // Set the arrow to point down but don't change position of ArrowX
      else if (arrowPointingUpDown == 0 && UserInput == KEY_LEFT) // The arrow points up and the user input is "turn to the left"
      {
        if (arrowX > 0)
          arrowX--; // Move arrow one postion to the left
      }
      else if (arrowPointingUpDown == 1 && UserInput == KEY_RIGHT) // The arrow points down and the user input is "turn to the right"
      {
        if (arrowX < 19)
          arrowX++; // Move arrow one postion to the right
      }
      else if (arrowPointingUpDown == 1 && UserInput == KEY_LEFT) // The arrow points down and the user input is "turn to the left"
        arrowPointingUpDown = 0;                                  // Set the arrow to point up but don't change position of ArrowX

      // Display arrow
      lcd.setCursor(arrowX, 2);
      if (arrowPointingUpDown == 1) // if arrow == 1, then print arrow that points down; if arrow == 0, then print arrow that points up
        lcd.write(27);
      else
        lcd.write(26);
      lcd.setCursor(9 + newInputName.length(), 0);
      lcd.BlinkingCursorOn();
      break;
    case KEY_SELECT:
      lcd.BlinkingCursorOff();
      if (arrowPointingUpDown == 1) // If arrow points down
      {
        if (arrowX == 17) // Switch between Upper and Lower case characters
        {
          isUpperCase = !isUpperCase;
          drawEditInputNameScreen(isUpperCase);
        }
        else if (arrowX == 18) // Back Space (the Backspace icon has been selected)
        {
          if (newInputName.length() > 0) // Make sure there is a character to delete!
          {
            lcd.setCursor(9 + newInputName.length() - 1, 0);
            lcd.print(" "); // Print to clear the deleted character on the display
            newInputName = newInputName.substring(0, newInputName.length() - 1);
          }
        }
        else if (arrowX == 19) // Done editing (the Enter icon has been selected)
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
          complete = true;
        }
        else if (newInputName.length() < 10) // Only allow up to 10 characters
        {
          byte v;
          if (isUpperCase)
            v = 84;
          else
            v = 116;
          if (arrowX > 6)
            v = 41;
          newInputName = newInputName + (char(arrowX + v));
        }
      }
      else // Arrow points up
      {
        if (newInputName.length() < 10) // Only allow up to 10 characters
        {
          if (arrowX == 0) // Space character has been selected (though it is shown as underscore)
            newInputName = newInputName + " ";
          else // A character between A-S has been selected, so add it to the string
          {
            if (isUpperCase)
              newInputName = newInputName + (char(arrowX + 64));
            else
              newInputName = newInputName + (char(arrowX + 96));
          }
        }
      }
      if (!complete)
      {
        lcd.setCursor(9, 0);
        lcd.print(newInputName);
        lcd.BlinkingCursorOn();
      }
      break;
    case KEY_BACK:
      // Exit without saving new value
      complete = true;
      break;
    default:
      break;
    }
  }
  lcd.BlinkingCursorOff();
}

void drawEditInputNameScreen(bool isUpperCase)
{
  lcd.setCursor(0, 1);
  lcd.write(byte(196)); // Print underscore to indicate Space
  if (isUpperCase)
  {
    for (int i = 65; i < 84; i++) // Print A-S
      lcd.write(i);
    lcd.setCursor(0, 3);
    for (int i = 84; i < 91; i++) // Print T-Z
      lcd.write(i);
  }
  else
  {
    for (int i = 97; i < 116; i++) // Print a-s
      lcd.write(i);
    lcd.setCursor(0, 3);
    for (int i = 116; i < 123; i++) // Print t-z
      lcd.write(i);
  }
  for (int i = 48; i < 58; i++) // Print 0-9
    lcd.write(i);
  if (isUpperCase)
    lcd.write(19);
  else
    lcd.write(18);
  lcd.write(225); // "Backspace" icon
  lcd.write(28);  // "Enter" icon
}

bool editNumericValue(byte &Value, byte MinValue, byte MaxValue)
{
  bool complete = false;
  bool result = false;
  char nameBuf[11];

  byte NewValue = Value;

  // Display the screen
  lcd.clear();
  lcd.print(Menu1.getCurrentItemName(nameBuf));
  lcd.setCursor(0, 2);
  lcd.print("Min. ");
  lcd.print(MinValue);
  lcd.setCursor(0, 3);
  lcd.print("Max. ");
  lcd.print(MaxValue);
  lcd.printTwoNumber(11, NewValue);

  while (!complete)
  {
    mil_LastUserInput = millis(); // Prevent the screen saver to kick in while editing
    switch (getUserInput())
    {
    case KEY_RIGHT:
      if (NewValue < MaxValue)
      {
        NewValue++;
        lcd.printTwoNumber(11, NewValue);
      }
      break;
    case KEY_LEFT:
      if (NewValue > MinValue)
      {
        NewValue--;
        lcd.printTwoNumber(11, NewValue);
      }
      break;
    case KEY_SELECT:
      Value = NewValue;
      writeSettingsToEEPROM();
      result = true;
      complete = true;
      break;
    case KEY_BACK:
      // Exit without saving new value
      result = false;
      complete = true;
      break;
    default:
      break;
    }
  }
  return result;
}

bool editOptionValue(byte &Value, byte NumOptions, const char Option1[9], const char Option2[9], const char Option3[9], const char Option4[9])
{
  bool complete = false;
  bool result = false;
  char nameBuf[11];

  byte NewValue = Value;

  // Display the screen
  lcd.clear();
  lcd.print(Menu1.getCurrentItemName(nameBuf));
  if (NumOptions < 3)
  {
    lcd.setCursor(1, 2);
    lcd.print(Option1);
    lcd.setCursor(11, 2);
    lcd.print(Option2);
  }
  else
  {
    lcd.setCursor(1, 2);
    lcd.print(Option1);
    lcd.setCursor(11, 2);
    lcd.print(Option2);
    lcd.setCursor(1, 3);
    lcd.print(Option3);
    lcd.setCursor(11, 3);
    lcd.print(Option4);
  }

  lcd.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
  lcd.write(16);

  while (!complete)
  {
    mil_LastUserInput = millis(); // Prevent the screen saver to kick in while editing
    switch (getUserInput())
    {
    case KEY_RIGHT:
      lcd.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
      lcd.print(" ");
      if (NewValue < NumOptions - 1)
        NewValue++;
      else
        NewValue = 0;
      lcd.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
      lcd.write(16);
      break;
    case KEY_LEFT:
      lcd.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
      lcd.print(" ");
      if (NewValue == 0)
        NewValue = NumOptions - 1;
      else
        NewValue--;
      lcd.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
      lcd.write(16);
      break;
    case KEY_SELECT:
      Value = NewValue;
      writeSettingsToEEPROM();
      result = true;
      complete = true;
      break;
    case KEY_BACK:
      // Exit without saving new value
      result = false;
      complete = true;
      break;
    default:
      break;
    }
  }
  return result;
}

bool editIRCode(HashIR_data_t &Value)
{
  bool complete = false;
  bool result = false;
  char nameBuf[11];

  HashIR_data_t NewValue, OldValue;
  NewValue.address = 0;
  NewValue.command = 0;

  // Display the screen
  lcd.clear();
  lcd.print(F("IR key "));
  lcd.print(Menu1.getCurrentItemName(nameBuf));

  lcd.setCursor(0, 1);
  lcd.print(F("Current:"));
  lcd.setCursor(0, 2);
  lcd.print(Value.address, HEX);
  lcd.setCursor(0, 3);
  lcd.print(Value.command, HEX);
  lcd.setCursor(10, 1);
  lcd.print(F("New:"));
  lcd.setCursor(10, 2);
  lcd.print(NewValue.address, HEX);
  lcd.setCursor(10, 3);
  lcd.print(NewValue.command, HEX);

  // As we don't want to react to received IR code while learning new code we temporaryly disable the current code in the CurrentSettings (we save a copy in OldValue)
  OldValue.address = Value.address;
  OldValue.command = Value.command;
  Value.address = 0x00;
  Value.command = 0x00;

  while (!complete)
  {
    mil_LastUserInput = millis(); // Prevent the screen saver to kick in while editing
    switch (getUserInput())
    {
    case KEY_SELECT:
      Value.address = NewValue.address;
      Value.command = NewValue.command;
      writeSettingsToEEPROM();
      result = true;
      complete = true;
      break;
    case KEY_BACK:
      // Exit without saving new value, but restore the original one
      Value.address = OldValue.address;
      Value.command = OldValue.command;
      result = false;
      complete = true;
      break;
    default:
      break;
    }
    if (IRLremote.available())
    {
      // Get the new data from the remote
      NewValue = IRLremote.read();
      lcd.setCursor(10, 2);
      lcd.print(F("          "));
      lcd.setCursor(10, 2);
      lcd.print(NewValue.address, HEX);
      lcd.setCursor(10, 3);
      lcd.print(F("          "));
      lcd.setCursor(10, 3);
      lcd.print(NewValue.command, HEX);
    }
  }
  return result;
}

// Loads default settings into CurrentSettings and CurrentRuntimeSettings - this is only done when the EEPROM does not contain valid settings or when reset is chosen by user in the menu
void setCurrentSettingsToDefault()
{
  CurrentSettings.VolumeSteps = 24;
  CurrentSettings.MinVolume = 0;
  CurrentSettings.MaxVolume = CurrentSettings.VolumeSteps;
  CurrentSettings.MaxStartVolume = CurrentSettings.VolumeSteps;
  CurrentSettings.MuteLevel = 0;
  CurrentSettings.RecallSetLevel = true;
  CurrentSettings.IR_UP.address = 0x24;
  CurrentSettings.IR_UP.command = 0x3AEA5A5F;
  CurrentSettings.IR_DOWN.address = 0x24;
  CurrentSettings.IR_DOWN.command = 0xE64E6057;
  CurrentSettings.IR_REPEAT.address = 0x00;
  CurrentSettings.IR_REPEAT.command = 0x00;
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
  CurrentSettings.IR_PREVIOUS.address = 0x24;
  CurrentSettings.IR_PREVIOUS.command = 0x5A3E996B;
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
  CurrentSettings.Input[1].Active = true;
  strcpy(CurrentSettings.Input[1].Name, "Input 2   ");
  CurrentSettings.Input[1].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[1].MinVol = 0;
  CurrentSettings.Input[2].Active = true;
  strcpy(CurrentSettings.Input[2].Name, "Input 3   ");
  CurrentSettings.Input[2].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[2].MinVol = 0;
  CurrentSettings.Input[3].Active = true;
  strcpy(CurrentSettings.Input[3].Name, "Input 4   ");
  CurrentSettings.Input[3].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[3].MinVol = 0;
  CurrentSettings.Input[4].Active = true;
  strcpy(CurrentSettings.Input[4].Name, "Input 5   ");
  CurrentSettings.Input[4].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[4].MinVol = 0;
  CurrentSettings.Input[5].Active = true;
  strcpy(CurrentSettings.Input[5].Name, "Input 6   ");
  CurrentSettings.Input[5].MaxVol = CurrentSettings.VolumeSteps;
  CurrentSettings.Input[5].MinVol = 0;
  CurrentSettings.Trigger1Type = false;
  CurrentSettings.Trigger1Mode = true;
  CurrentSettings.Trigger1OnDelay = 10;
  CurrentSettings.Trigger1Temp = 60;
  CurrentSettings.Trigger2Type = false;
  CurrentSettings.Trigger2Mode = true;
  CurrentSettings.Trigger2OnDelay = 10;
  CurrentSettings.Trigger2Temp = 60;
  CurrentSettings.TriggerInactOffTimer = 0;
  CurrentSettings.ScreenSaverActive = true;
  CurrentSettings.DisplayOnLevel = 3;
  CurrentSettings.DisplayDimLevel = 0;
  CurrentSettings.DisplayTimeout = 30;
  CurrentSettings.DisplaySelectedInput = true;
  CurrentSettings.DisplayTemperature1 = 3;
  CurrentSettings.DisplayTemperature2 = 3;
  CurrentSettings.Version = VERSION;

  CurrentRuntimeSettings.CurrentInput = 0;
  CurrentRuntimeSettings.CurrentVolume = 0;
  CurrentRuntimeSettings.Muted = 0;
  CurrentRuntimeSettings.InputLastVol[0] = 0;
  CurrentRuntimeSettings.InputLastVol[1] = 0;
  CurrentRuntimeSettings.InputLastVol[2] = 0;
  CurrentRuntimeSettings.InputLastVol[3] = 0;
  CurrentRuntimeSettings.InputLastVol[4] = 0;
  CurrentRuntimeSettings.InputLastVol[5] = 0;
  CurrentRuntimeSettings.PrevSelectedInput = 0;
  CurrentRuntimeSettings.Version = VERSION;
}

// Write CurrentSettings to EEPROM
void writeSettingsToEEPROM()
{
  // Write the settings to the EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.write(0, CurrentSettings.data, sizeof(CurrentSettings));
}

// Read CurrentSettings from EEPROM
void readSettingsFromEEPROM()
{
  // Read settings from EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.read(0, CurrentSettings.data, sizeof(CurrentSettings));
}

// Write Default Settings and RuntimeSettings to EEPROM - called if the EEPROM data is not valid or if the user chooses to reset all settings to default value
void writeDefaultSettingsToEEPROM()
{
  // Read default settings into CurrentSettings
  setCurrentSettingsToDefault();
  // Write the settings to the EEPROM
  writeSettingsToEEPROM();
  // Write the runtime settings to the EEPROM
  writeRuntimeSettingsToEEPROM();
}

// Write the current runtime settings to EEPROM - called if a power drop is detected or if the EEPROM data is not valid or if the user chooses to reset all settings to default values
void writeRuntimeSettingsToEEPROM()
{
  // Write the settings to the EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.write(sizeof(CurrentSettings) + 1, CurrentRuntimeSettings.data, sizeof(CurrentRuntimeSettings));
}

// Read the last runtime settings from EEPROM
void readRuntimeSettingsFromEEPROM()
{
  // Read the settings from the EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.read(sizeof(CurrentSettings) + 1, CurrentRuntimeSettings.data, sizeof(CurrentRuntimeSettings));
}

// Reboots the sketch - used after restoring default settings
void reboot()
{
  // TO DO Unselect all inputs (unless all inputs are deactivated by the MCP23008 during reboot - has to be tested)
  // TO DO Mute volume control)
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("REBOOTING...");
  delay(2000);
  lcd.clear();
  asm volatile("  jmp 0"); // Restarts the sketch
}
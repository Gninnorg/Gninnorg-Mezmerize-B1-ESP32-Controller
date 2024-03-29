/*
**
**    Controller for Mezmerize B1 Buffer using Muses72320 potentiometer
**
**    Copyright (c) 2019-2023 Carsten Grønning, Jan Abkjer Tofft
**
**    2019-2023
**
*/

#define VERSION (float)0.99

// To enable debug define DEBUG 1
// To disable debug define DEBUG 2
#define DEBUG 1
#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

// ADC calibration factor for ESP32 used:
// Carsten (1.045)
// Jan (1.085)
#define ADC_CALIBRATION 1.045 


#include <Wire.h>
#include <Adafruit_MCP23008.h>
#include <OLedI2C.h>
#include <extEEPROM.h>
#include <Muses72320.h>
#include <MenuManager.h>
#include <MenuData.h>
#include <esp_adc_cal.h> // To enable improved accuracy of ADC readings (used for reading NTC's value to calculate temperature)

#undef minimum
#ifndef minimum
#define minimum(a, b) ((a) < (b) ? (a) : (b))
#endif

#include <ClickEncoder.h>
#define ROTARY_ENCODER_STEPS 4

#define IRMP_INPUT_PIN 32
#define NTC1_PIN 35
#define NTC2_PIN 34
#define ROTARY2_CW_PIN 14
#define ROTARY2_CCW_PIN 12
#define ROTARY2_SW_PIN 13
#define ROTARY1_CW_PIN 25
#define ROTARY1_CCW_PIN 26
#define ROTARY1_SW_PIN 27
#define POWER_RELAY_PIN 4

#include <irmpSelectMain15Protocols.h> // This enables 15 main protocols
#define IRMP_SUPPORT_NEC_PROTOCOL 1    // this enables only one protocol

#include <irmp.hpp>

// Declarations
void startUp(void);
void toStandbyMode(void);
void toAppNormalMode(void);
void ScreenSaverOff(void);
void setTrigger1On(void);
void setTrigger2On(void);
void setTrigger1Off(void);
void setTrigger2Off(void);
void displayTemperatures(void);
void displayTempDetails(float, uint8_t, uint8_t, uint8_t);
float readVoltage(byte);
float getTemperature(uint8_t);
void displayVolume(void);
void displayMute(void);
void displayInput(void);
int16_t getAttenuation(uint8_t, uint8_t, uint8_t, uint8_t);
void setVolume(int16_t);
bool changeBalance(void);
void displayBalance(byte);
void mute(void);
void unmute(void);
boolean setInput(uint8_t);
void setPrevInput(void);
void setNextInput(void);
String getJSONCurrentValues(void);
String getJSONOnStandbyState(void);
String getJSONCurrentInput(void);
String getJSONCurrentVolume(void);
String getJSONTempValues(void);
void readSettingsFromEEPROM(void);
void writeSettingsToEEPROM(void);
void writeDefaultSettingsToEEPROM(void);
void readRuntimeSettingsFromEEPROM(void);
void writeRuntimeSettingsToEEPROM(void);
void readUserSettingsFromEEPROM(void);
void writeUserSettingsToEEPROM(void);
void editInputName(uint8_t InputNumber);
void drawEditInputNameScreen(bool isUpperCase);
bool editNumericValue(byte &Value, byte MinValue, byte MaxValue, const char Unit[5]);
bool editOptionValue(byte &Value, byte NumOptions, const char Option1[9], const char Option2[9], const char Option3[9], const char Option4[9]);

IRMP_DATA irmp_data;
bool editIRCode(IRMP_DATA &Value);

void drawMenu();
void refreshMenuDisplay(byte refreshMode);
byte processMenuCommand(byte cmdId);
byte getNavAction();

byte menuIndex = 0;

unsigned long mil_On = millis(); // Holds the millis from last power on (or restart)

#define INPUT_HT_PASSTHROUGH 0
#define INPUT_NORMAL 1
#define INPUT_INACTIVATED 2

#define ABANDON 99

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
typedef union
{
  struct
  {
    char ssid[33];    // Wifi network SSDI
    char pass[33];    // Wifi network password
    char ip[16];      // Wifi network assigned IP address
    char gateway[16]; // Wifi network gateway IP address

    byte VolumeSteps;    // The number of steps of the volume control
    byte MinAttenuation; // Minimum attenuation in -dB (as 0 db equals no attenuation this is equal to the highest volume allowed)
    byte MaxAttenuation; // Maximum attenuation in -dB (as -111.5 db is the limit of the Muses72320 this is equal to the lowest volume possible). We only keep this setting as a positive number, and we do also only allow the user to set the value in 1 dB steps
    byte MaxStartVolume; // If StoreSetLevel is true, then limit the volume to the specified value when the controller is powered on
    byte MuteLevel;      // The level to be set when Mute is activated by the user. The Mute function of the Muses72320 is activated if 0 is specified
    byte RecallSetLevel; // Remember/store the volume level for each separate input

    float ADC_Calibration; // Used for calibration of the ADC readings when reading temperatures from the attached NTCs. The value differs (quite a lot) between ESP32's

    IRMP_DATA IR_ONOFF;            // IR data to be interpreted as ON/OFF - switch between running and suspend mode (and turn triggers off)
    IRMP_DATA IR_UP;               // IR data to be interpreted as UP
    IRMP_DATA IR_DOWN;             // IR data to be interpreted as DOWN
    IRMP_DATA IR_REPEAT;           // IR data to be interpreted as REPEAT (ie Apple remotes sends a specific code, if a key is held down to indicate repeat of the previously sent code
    IRMP_DATA IR_LEFT;             // IR data to be interpreted as LEFT
    IRMP_DATA IR_RIGHT;            // IR data to be interpreted as RIGHT
    IRMP_DATA IR_SELECT;           // IR data to be interpreted as SELECT
    IRMP_DATA IR_BACK;             // IR data to be interpreted as BACK
    IRMP_DATA IR_MUTE;             // IR data to be interpreted as MUTE
    IRMP_DATA IR_PREVIOUS;         // IR data to be interpreted as "switch to previous selected input"
    IRMP_DATA IR_1;                // IR data to be interpreted as 1 (to select input 1 directly)
    IRMP_DATA IR_2;                // IR data to be interpreted as 2
    IRMP_DATA IR_3;                // IR data to be interpreted as 3
    IRMP_DATA IR_5;                // IR data to be interpreted as 5
    IRMP_DATA IR_4;                // IR data to be interpreted as 4
    IRMP_DATA IR_6;                // IR data to be interpreted as 6
    struct InputSettings Input[6]; // Settings for all 6 inputs
    bool ExtPowerRelayTrigger;     // Enable triggering of relay for external power (we use it to control the power of the Mezmerize)
    byte Trigger1Active;           // 0 = the trigger is not active, 1 = the trigger is active
    byte Trigger1Type;             // 0 = momentary, 1 = latching
    byte Trigger1OnDelay;          // Seconds from controller power up to activation of trigger. The default delay allows time for the output relay of the Mezmerize to be activated before we turn on the power amps. The selection of an input of the Mezmerize will also be delayed.
    byte Trigger1Temp;             // Temperature protection: if the temperature is measured to the set number of degrees Celcius (via the LDRs), the controller will attempt to trigger a shutdown of the connected power amps (if set to 0, the temperature protection is not active
    byte Trigger2Active;           // 0 = the trigger is not active, 1 = the trigger is active
    byte Trigger2Type;             // 0 = momentary, 1 = latching
    byte Trigger2OnDelay;          // Seconds from controller power up to activation of trigger. The default delay allows time for the output relay of the Mezmerize to be activated before we turn on the power amps. The selection of an input of the Mezmerize will also be delayed.
    byte Trigger2Temp;             // Temperature protection: if the temperature is measured to the set number of degrees Celcius (via the LDRs), the controller will attempt to trigger a shutdown of the connected power amps (if set to 0, the temperature protection is not active)
    byte TriggerInactOffTimer;     // Hours without user interaction before automatic power down (0 = never)
    byte ScreenSaverActive;        // 0 = the display will stay on/not be dimmed, 1 = the display will be dimmed to the specified level after a specified period of time with no user input
    byte DisplayOnLevel;           // The contrast level of the display when it is on, 0 = 25%, 1 = 50%, 2 = 75%, 3 = 100%
    byte DisplayDimLevel;          // The contrast level of the display when screen saver is active. 0 = off, 1 = 3, 2 = 7 ... 32 = 127. If DisplayDimLevel = 0 the display will be turned off when the screen saver is active (to reduce electrical noise)
    byte DisplayTimeout;           // Number of seconds before the screen saver is activated.
    byte DisplayVolume;            // 0 = no display of volume, 1 = show step number, 2 = show as -dB
    byte DisplaySelectedInput;     // 0 = the name of the active input is not shown on the display (ie. if only one input is used), 1 = the name of the selected input is shown on the display
    byte DisplayTemperature1;      // 0 = do not display the temperature measured by NTC 1, 1 = display in number of degrees Celcious, 2 = display as graphical representation, 3 = display both
    byte DisplayTemperature2;      // 0 = do not display the temperature measured by NTC 2, 1 = display in number of degrees Celcious, 2 = display as graphical representation, 3 = display both
    float Version;                 // Used to check if data read from the EEPROM is valid with the compiled version of the code - if not a reset to default settings is necessary and they must be written to the EEPROM
  };
  byte data[232]; // Allows us to be able to write/read settings from EEPROM byte-by-byte (to avoid specific serialization/deserialization code)
} mySettings;

mySettings Settings; // Holds all the current settings
void setSettingsToDefault(void);

typedef union
{
  struct
  {
    byte CurrentInput;      // The number of the currently set input
    byte CurrentVolume;     // The currently set volume
    bool Muted;             // Indicates if we are in mute mode or not
    byte InputLastVol[6];   // The last set volume for each input
    byte InputLastBal[6];   // The last set balance for each input: 127 = no balance shift (values < 127 = shift balance to the left channel, values > 127 = shift balance to the right channel)
    byte PrevSelectedInput; // Holds the input selected before the current one (enables switching back and forth between two inputs, eg. while A-B testing)
    float Version;          // Used to check if data read from the EEPROM is valid with the compiled version of the compiled code - if not a reset to defaults is necessary and they must be written to the EEPROM
  };
  byte data[20]; // Allows us to be able to write/read settings from EEPROM byte-by-byte (to avoid specific serialization/deserialization code)
} myRuntimeSettings;

myRuntimeSettings RuntimeSettings;

// Setup Rotary encoders ------------------------------------------------------
ClickEncoder *encoder1 = new ClickEncoder(ROTARY1_CW_PIN, ROTARY1_CCW_PIN, ROTARY1_SW_PIN, ROTARY_ENCODER_STEPS, LOW);
ClickEncoder::Button button1;
int16_t e1last, e1value;

ClickEncoder *encoder2 = new ClickEncoder(ROTARY2_CW_PIN, ROTARY2_CCW_PIN, ROTARY2_SW_PIN, ROTARY_ENCODER_STEPS, LOW);
ClickEncoder::Button button2;
int16_t e2last, e2value;

volatile int interruptCounter;
int totalInterruptCounter;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
void IRAM_ATTR timerIsr()
{
  encoder1->service();
  encoder2->service();
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setupRotaryEncoders()
{
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &timerIsr, true);
  timerAlarmWrite(timer, 1000, true);
  timerAlarmEnable(timer);
}

// Setup Muses72320 -----------------------------------------------------------
Muses72320 muses(0);

// Setup Relay Controller------------------------------------------------------
Adafruit_MCP23008 relayController;

// Setup EEPROM ---------------------------------------------------------------
#define EEPROM_Address 0x50
extEEPROM eeprom(kbits_64, 1, 32); // Set to use 24C64 Eeprom - if you use another type look in the datasheet for capacity in kbits (kbits_64) and page size in bytes (32)

// Setup Display ---------------------------------------------------------------
OLedI2C oled;
// Used to indicate whether the screen saver is running or not
bool ScreenSaverIsOn = false;
// Used to keep track of the time of the last user interaction (part of the screen saver timing)
unsigned long mil_LastUserInput = millis();
// Used to time how often the display of temperatures is updated
unsigned long mil_onRefreshTemperatureDisplay;
// Update interval for the display/notification of temperatures
#define TEMP_REFRESH_INTERVAL 10000         // Interval while on
#define TEMP_REFRESH_INTERVAL_STANDBY 60000 // Interval while in standby

//  Initialize the menu
enum AppModeValues
{
  APP_NORMAL_MODE,
  APP_BALANCE_MODE,
  APP_MENU_MODE,
  APP_PROCESS_MENU_CMD,
  APP_STANDBY_MODE
};

byte appMode = APP_NORMAL_MODE;

MenuManager Menu1(ctlMenu_Root, menuCount(ctlMenu_Root));

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

// Returns input from the user - enumerated to be the same value no matter if input is from encoders or IR remote
byte getUserInput()
{

  if (interruptCounter > 0)
  {
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);

    totalInterruptCounter++;
  }

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
  if (irmp_get_data(&irmp_data))
  {

    // Often the IR remote is to sensitive, reset reading if its to fast, but only if IR code is not REPEAT
    if (millis() - mil_LastUserInput < 100 && (irmp_data.address != Settings.IR_REPEAT.address && irmp_data.command != Settings.IR_REPEAT.command))
    {
      irmp_data.address = 0;
      irmp_data.command = 0;
      receivedInput = KEY_NONE;
    }
    else
      // Map the received IR input to UserInput values
      if (irmp_data.address == Settings.IR_UP.address && irmp_data.command == Settings.IR_UP.command)
        receivedInput = KEY_UP;
      else if (irmp_data.address == Settings.IR_DOWN.address && irmp_data.command == Settings.IR_DOWN.command)
        receivedInput = KEY_DOWN;
      else if (irmp_data.address == Settings.IR_LEFT.address && irmp_data.command == Settings.IR_LEFT.command)
        receivedInput = KEY_LEFT;
      else if (irmp_data.address == Settings.IR_RIGHT.address && irmp_data.command == Settings.IR_RIGHT.command)
        receivedInput = KEY_RIGHT;
      else if (irmp_data.address == Settings.IR_SELECT.address && irmp_data.command == Settings.IR_SELECT.command)
        receivedInput = KEY_SELECT;
      else if (irmp_data.address == Settings.IR_BACK.address && irmp_data.command == Settings.IR_BACK.command)
        receivedInput = KEY_BACK;
      else if (irmp_data.address == Settings.IR_MUTE.address && irmp_data.command == Settings.IR_MUTE.command)
        receivedInput = KEY_MUTE;
      else if (irmp_data.address == Settings.IR_ONOFF.address && irmp_data.command == Settings.IR_ONOFF.command)
        receivedInput = KEY_ONOFF;
      else if (irmp_data.address == Settings.IR_1.address && irmp_data.command == Settings.IR_1.command)
        receivedInput = KEY_1;
      else if (irmp_data.address == Settings.IR_2.address && irmp_data.command == Settings.IR_2.command)
        receivedInput = KEY_2;
      else if (irmp_data.address == Settings.IR_3.address && irmp_data.command == Settings.IR_3.command)
        receivedInput = KEY_3;
      else if (irmp_data.address == Settings.IR_4.address && irmp_data.command == Settings.IR_4.command)
        receivedInput = KEY_4;
      else if (irmp_data.address == Settings.IR_5.address && irmp_data.command == Settings.IR_5.command)
        receivedInput = KEY_5;
      else if (irmp_data.address == Settings.IR_6.address && irmp_data.command == Settings.IR_6.command)
        receivedInput = KEY_6;
      else if (irmp_data.address == Settings.IR_PREVIOUS.address && irmp_data.command == Settings.IR_PREVIOUS.command)
        receivedInput = KEY_PREVIOUS;
      else if (irmp_data.address == Settings.IR_REPEAT.address && irmp_data.command == Settings.IR_REPEAT.command)
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
        toStandbyMode();
      }
      else // wake from standby
        startUp();
    }
  }

  // Turn Screen Saver on/off if it is activated and if no user input has been received during the defined number of seconds
  if (receivedInput == KEY_NONE)
  {
    if ((!ScreenSaverIsOn && (millis() - mil_LastUserInput > (unsigned long)Settings.DisplayTimeout * 1000)) && Settings.ScreenSaverActive)
    {
      if (Settings.DisplayDimLevel == 0)
        oled.lcdOff();
      else
        oled.backlight(Settings.DisplayDimLevel * 4 - 1);
      ScreenSaverIsOn = true;
    }
  }
  else if (appMode != APP_STANDBY_MODE)
    ScreenSaverOff();

  // If inactivity timer is set, go to standby if the set number of hours have passed since last user input
  if ((appMode != APP_STANDBY_MODE) && (Settings.TriggerInactOffTimer > 0) && ((mil_LastUserInput + Settings.TriggerInactOffTimer * 3600000) < millis()))
  {
    toStandbyMode();
  }

  return (receivedInput);
}

void ScreenSaverOff()
{
  mil_LastUserInput = millis();
  if (ScreenSaverIsOn)
  {
    oled.lcdOn();
    if (Settings.DisplayDimLevel != 0)
      oled.backlight((Settings.DisplayOnLevel + 1) * 64 - 1);
    ScreenSaverIsOn = false;
  }
}

// Webserver & Websocket
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
// #include <ArduinoJson.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Get all current values as JSON
String getJSONCurrentValues()
{
  JSONVar JSONValues; // Json variable to hold values
  if (appMode == APP_STANDBY_MODE)
    JSONValues["OnState"] = "Standby";
  else
    JSONValues["OnState"] = "On";
  JSONValues["Input"] = String(Settings.Input[RuntimeSettings.CurrentInput].Name);
  JSONValues["VolumeSteps"] = Settings.VolumeSteps;
  JSONValues["Volume"] = RuntimeSettings.CurrentVolume;

  int Attenuation = getAttenuation(Settings.VolumeSteps, RuntimeSettings.CurrentVolume, Settings.MinAttenuation, Settings.MaxAttenuation);

  JSONValues["Volume"] = RuntimeSettings.CurrentVolume;
  if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] == 127 || RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] < 118 || RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] > 136)
    JSONValues["Volume_dB"] = String(float(Attenuation / 2));
  else if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] < 127) // Balance shifted to the left channel by lowering the right channel
    JSONValues["Volume_dB"] = String(float(Attenuation / 2)) + String("/") + String(float((Attenuation + (127 - RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput])) / 2));
  else if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] > 127) // Balance shifted to the right channel by lowering the left channel
    JSONValues["Volume_dB"] = String(float((Attenuation + (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] - 127)) / 2)) + String("/") + String(float(Attenuation / 2));

  JSONValues["Temp1"] = int(getTemperature(NTC1_PIN));
  JSONValues["Temp2"] = int(getTemperature(NTC2_PIN));
  return JSON.stringify(JSONValues);
}

// Get On/Standby state as JSON
String getJSONOnStandbyState()
{
  JSONVar JSONValues; // Json variable to hold values
  if (appMode == APP_STANDBY_MODE)
    JSONValues["OnState"] = "Standby";
  else
    JSONValues["OnState"] = "On";
  return JSON.stringify(JSONValues);
}

// Get selected input name as JSON
String getJSONCurrentInput()
{
  JSONVar JSONValues; // Json variable to hold values
  JSONValues["Input"] = String(Settings.Input[RuntimeSettings.CurrentInput].Name);
  return JSON.stringify(JSONValues);
}

// Get current volume as JSON
String getJSONCurrentVolume()
{
  JSONVar JSONValues; // Json variable to hold values
  int Attenuation = getAttenuation(Settings.VolumeSteps, RuntimeSettings.CurrentVolume, Settings.MinAttenuation, Settings.MaxAttenuation);

  JSONValues["Volume"] = RuntimeSettings.CurrentVolume;
  if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] == 127 || RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] < 118 || RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] > 136)
    JSONValues["Volume_dB"] = String(float(Attenuation / 2));
  else if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] < 127) // Balance shifted to the left channel by lowering the right channel
    JSONValues["Volume_dB"] = String(float(Attenuation / 2)) + String("/") + String(float((Attenuation + (127 - RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput])) / 2));
  else if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] > 127) // Balance shifted to the right channel by lowering the left channel
    JSONValues["Volume_dB"] = String(float((Attenuation + (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] - 127)) / 2)) + String("/") + String(float(Attenuation / 2));
  return JSON.stringify(JSONValues);
}

// Get temperatures as JSON
String getJSONTempValues()
{
  JSONVar JSONValues; // Json variable to hold values
  JSONValues["Temp1"] = int(getTemperature(NTC1_PIN));
  JSONValues["Temp2"] = int(getTemperature(NTC2_PIN));
  return JSON.stringify(JSONValues);
}

void notifyClients(String message)
{
  ws.textAll(message);
  debugln("Sent: " + message);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    String message = (char *)data;

    if (message.indexOf("Volume:Up") >= 0)
      setVolume(RuntimeSettings.CurrentVolume + 1);
    else if (message.indexOf("Volume:Down") >= 0)
      setVolume(RuntimeSettings.CurrentVolume - 1);
    else if (message.indexOf("Volume:") >= 0)
      setVolume(message.substring(7).toInt());

    if (message.indexOf("Input:Up") >= 0)
      setNextInput();
    else if (message.indexOf("Input:Down") >= 0)
      setPrevInput();
    else if (message.indexOf("Input:") >= 0)
      setInput(message.substring(7).toInt());

    if (message.indexOf("Power:") >= 0)
    {
      if (appMode == APP_STANDBY_MODE && message.indexOf("Power:On") >= 0) startUp();
      if (appMode == APP_NORMAL_MODE && message.indexOf("Power:Standby") >= 0) toStandbyMode();
      if (message.indexOf("Power:Toggle") >= 0)
      {
        if (appMode == APP_STANDBY_MODE) startUp();
        else if (appMode == APP_NORMAL_MODE) toStandbyMode();
      }
      notifyClients(getJSONOnStandbyState());
    }



    // Default message received when a new Websocket client connects -> Send all values
    if (strcmp((char *)data, "getValues") == 0)
    {
      notifyClients(getJSONCurrentValues());
    }
  }
  mil_LastUserInput = millis();
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// Search for parameter in HTTP POST request - used for wifi configuration page
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";

IPAddress localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
// IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)

// Initialize SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    debugln("An error has occurred while mounting SPIFFS");
  }
  debugln("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    debugln("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    debugln("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    debugln("- file written");
  }
  else
  {
    debugln("- write failed");
  }
}

// Initialize WiFi
bool initWiFi()
{
  if (Settings.ssid == "" || Settings.ip == "")
  {
    debugln("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(Settings.ip);
  localGateway.fromString(Settings.gateway);

  if (!WiFi.config(localIP, localGateway, subnet))
  {
    debugln("STA Failed to configure");
    return false;
  }
  WiFi.begin(Settings.ssid, Settings.pass);
  debugln("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
      debugln("Failed to connect.");
      return false;
    }
  }

  debugln(WiFi.localIP());
  return true;
}

// Replaces placeholders with values
String processor(const String &var)
{
  if (var == "STATE")
  {
    if (appMode != APP_STANDBY_MODE)
    {
      return "ON";
    }
    else
    {
      return "SLEEPING";
    }
  }
  if (var == "SELECTEDINPUT")
  {
    return String(Settings.Input[RuntimeSettings.CurrentInput].Name);
  }
  if (var == "VOLUME")
  {
    if (!RuntimeSettings.Muted)
      return String(RuntimeSettings.CurrentVolume) + " (" + String(getAttenuation(Settings.VolumeSteps, RuntimeSettings.CurrentVolume, Settings.MinAttenuation, Settings.MaxAttenuation) / 2) + " dB)";
    else
      return ("MUTED");
  }
  if (var == "TEMP1")
  {
    return String(getTemperature(NTC1_PIN));
  }
  if (var == "TEMP2")
  {
    return String(getTemperature(NTC2_PIN));
  }
  // Unknown parameter
  return ("n/a)");
}

void setupWIFIsupport()
{
  initSPIFFS();

  if (initWiFi())
  {
    initWebSocket();

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", "text/html"); });
    
    // Web : InputSelector
    server.on("/INPUT1", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(setInput(1)));});

    server.on("/INPUT2", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(setInput(2)));});

    server.on("/INPUT3", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(setInput(3)));});

    server.on("/INPUT4", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(setInput(4)));});

    server.on("/INPUT5", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(setInput(5)));});

    server.on("/INPUT6", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(setInput(6)));});
        

    server.serveStatic("/", SPIFFS, "/");

    AsyncElegantOTA.begin(&server);
    server.begin();
  }
  else
  {
    // Connect to Wi-Fi network with SSID and password
    debugln("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("PreAmp", NULL);

    IPAddress IP = WiFi.softAPIP();
    debug("AP IP address: ");
    debugln(IP);

    oled.clear();
    oled.setCursor(0, 1);
    oled.print(F("WiFi: PreAmp"));
    oled.setCursor(0, 2);
    oled.print(IP);
    delay(10000);

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/wifi.html", "text/html"); });

    server.serveStatic("/", SPIFFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            strcpy(Settings.ssid, p->value().c_str()); /* String copy*/
            debug("SSID set to: ");
            debugln(Settings.ssid);
            // Write file to save value
            //writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            strcpy(Settings.pass, p->value().c_str()); /* String copy */
            debug("Password set to: ");
            debugln(Settings.pass);
            // Write file to save value
            // writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            strcpy(Settings.ip, p->value().c_str()); /* String copy*/
            debug("IP Address set to: ");
            debugln(Settings.ip);
            // Write file to save value
            //writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            strcpy(Settings.gateway, p->value().c_str()); /* String copy */
            debug("Gateway set to: ");
            debugln(Settings.gateway);
            // Write file to save value
            //writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }

      writeSettingsToEEPROM();
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + String(Settings.ip));
      oled.clear();
      oled.setCursor(0, 1);
      oled.print(F("Wifi is configured"));
      oled.setCursor(0, 3);
      oled.print(F("Restarting..."));
      delay(3000);
      ESP.restart(); });
    AsyncElegantOTA.begin(&server);
    server.begin();
  }
}

// Webserver & Websocket - END

// Lets get started ----------------------------------------------------------------------------------------
void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  Wire.begin();

  setupRotaryEncoders();

  pinMode(NTC1_PIN, INPUT);
  pinMode(NTC2_PIN, INPUT);

  relayController.begin();
  // Define all pins as OUTPUT and disable all relais
  for (byte pin = 0; pin <= 7; pin++)
  {
    relayController.pinMode(pin, OUTPUT);
    relayController.digitalWrite(pin, LOW);
  }

  muses.begin();

  oled.begin();
  oled.backlight((Settings.DisplayOnLevel + 1) * 64 - 1);

  // Start IR reader
  irmp_init();

  // Read setting from EEPROM
  readSettingsFromEEPROM();
  readRuntimeSettingsFromEEPROM();

  // Check if settings stored in EEPROM are INVALID - if so, we write the default settings to the EEPROM and reboots
  if ((Settings.Version != (float)VERSION) || (RuntimeSettings.Version != (float)VERSION))
  {
    oled.clear();
    oled.setCursor(0, 1);
    oled.print("Restoring default");
    oled.setCursor(0, 2);
    oled.print(F("settings..."));
    delay(2000);
    writeDefaultSettingsToEEPROM();
  }

  // Connect to Wifi
  oled.clear();
  oled.setCursor(0, 1);
  oled.print("Connecting to Wifi");
  setupWIFIsupport();

  // Set pin mode for control of power relay
  pinMode(POWER_RELAY_PIN, OUTPUT);

  startUp();
}

void startUp()
{
  oled.lcdOn();
  oled.clear();

  // Turn on Mezmerize B1 Buffer via power on/off relay
  if (Settings.ExtPowerRelayTrigger)
  {
    digitalWrite(POWER_RELAY_PIN, HIGH);
  }

  // The controller is now ready - save the timestamp
  mil_On = millis();

  // If triggers are active then wait for the set number of seconds and turn them on
  unsigned long delayTrigger1 = (Settings.Trigger1Active) ? (mil_On + Settings.Trigger1OnDelay * 1000) : 0;
  unsigned long delayTrigger2 = (Settings.Trigger2Active) ? (mil_On + Settings.Trigger2OnDelay * 1000) : 0;

  if (delayTrigger1 || delayTrigger2)
  {
    oled.clear();
    oled.print(F("Wait..."));
  }

  while (delayTrigger1 || delayTrigger2)
  {
    if (millis() > delayTrigger1 && delayTrigger1 != 0)
    {
      setTrigger1On();
      delayTrigger1 = 0;
      // oled.print3x3Number(2, 1, 0, false);
    }
    else
    {
      if (Settings.Trigger1Active && delayTrigger1 != 0)
        oled.print3x3Number(2, 1, (delayTrigger1 - millis()) / 1000, false);
    }

    if (millis() > delayTrigger2 && delayTrigger2 != 0)
    {
      setTrigger2On();
      delayTrigger2 = 0;
      // oled.print3x3Number(11, 1, 0, false);
    }
    else
    {
      if (Settings.Trigger2Active && delayTrigger2 != 0)
        oled.print3x3Number(11, 1, (delayTrigger2 - millis()) / 1000, false);
    }
  }
  oled.clear();

  appMode = APP_NORMAL_MODE;
  // Keep start volume for current input lower hand max allowed start volume
  RuntimeSettings.InputLastVol[RuntimeSettings.CurrentInput] = minimum(RuntimeSettings.InputLastVol[RuntimeSettings.CurrentInput], Settings.MaxStartVolume); // Avoid setting volume higher than MaxStartVol
  setInput(RuntimeSettings.CurrentInput);
  displayTemperatures();
  unmute();

  UIkey = KEY_NONE;
  lastReceivedInput = KEY_NONE;

  notifyClients(getJSONOnStandbyState());
  
  debugln("Ready!");
}

void setTrigger1On()
{
  if (Settings.Trigger1Active)
  {
    relayController.digitalWrite(6, HIGH);
    if (Settings.Trigger1Type == 0) // Momentary
    {
      delay(200);
      relayController.digitalWrite(6, LOW);
    }
  }
}

void setTrigger2On()
{
  if (Settings.Trigger2Active)
  {
    relayController.digitalWrite(7, HIGH);
    if (Settings.Trigger2Type == 0) // Momentary
    {
      delay(200);
      relayController.digitalWrite(7, LOW);
    }
  }
}

void setTrigger1Off()
{
  if (Settings.Trigger1Active)
  {
    if (Settings.Trigger1Type == 0) // Momentary
    {
      relayController.digitalWrite(6, HIGH);
      delay(200);
    }
    relayController.digitalWrite(6, LOW);
  }
}

void setTrigger2Off()
{
  if (Settings.Trigger2Active)
  {
    if (Settings.Trigger2Type == 0) // Momentary
    {
      relayController.digitalWrite(7, HIGH);
      delay(200);
    }
    relayController.digitalWrite(7, LOW);
  }
}

int16_t getAttenuation(uint8_t steps, uint8_t selStep, uint8_t min_dB, uint8_t max_dB)
{
  /*
  ** Return the attenuation required by the setvolume function of the Muses72320 based upon the configured 
  ** number of steps, the selected step and the configured minimum and maximum attenuation in dBs.
  **
  ** The purpose of the algoritm is the divide the potentionmeter into to sections. To get a more even step
  ** size in terms of the experienced sound preassure. One section will have larger attenuation step sizes.
  ** This will be in section starting from the maximum attenuation. Another section will have smaller step sizes.
  ** This will be in section closer to the minimum attenuation.
  ** 
  ** Parameters
  **   steps   : Number of desired step for you potentiometer. 
  **             Maximum number of step = (max_dB-min_dB) * 2 
  **   selStep : Selected step in potentiometer ladder from which you wan't the attenuation calculated
  **             selStep = 0      (equals max_dB attenuation)
  **             selStep = steps  (equals min_db attenuation)
  **   min_dB  : Minimum attenuation for the potentiometer      0 dB = absolute minimum 
  **   max_dB  : Maximum attenuation for the potentionmeter   111 dB = absolute maximum
  **
  ** Constraints
  **   max_dB < min_dB
  **   selStep <= steps
  **   steps >= 10 
  **   steps <= (max_dB - min_dB) / 2
  **
  ** If the above constraints are not meet the getAttenuation() will return 223 (111.5 max attenuation);
  **
  */
  debug("steps: "); debug(steps); debug(" selectedStep: "); debug(selStep); debug(" min_dB: "); debug(min_dB); debug(" max_dB: "); debugln(max_dB);
  if (min_dB >= max_dB ||
      selStep > steps ||
      steps < 10 ||
      steps <= ((max_dB - min_dB) / 2)) return -223;

  // Calculate attenuation range in dB
  uint8_t att_dB = max_dB - min_dB;

  // Calculate step size in DB for attenuation steps
  float sizeOfMajorSteps = round(pow(2.0, att_dB / steps) - 0.5);
  float sizeOfMinorSteps = sizeOfMajorSteps / 2;

  // Calculate number of minor steps for section with minor steps 
  // Use as many steps as possible for minor steps
  uint8_t numberOfMinorSteps = (sizeOfMajorSteps * steps - att_dB) / sizeOfMinorSteps;

  // return calculated for total attenuation for selected step off_set equals min_db
  return minimum((min_dB + 
                  // total attenuation in dB from number of selected steps in minor step section
                  // Start using minor steps - we want as many of these as possible.
                  // Equals when attenuation is close to 0 the steps should be as fine as possible
                  minimum(steps - selStep, numberOfMinorSteps) * sizeOfMinorSteps +   
                  // total attenuation in dB from number of selected step in major step section
                  // When every minor step is used start using large steps
                  // Equals when attenuation is close to max remaing steps should be the major ones.
                  max(steps - numberOfMinorSteps - selStep, 0) * sizeOfMajorSteps),
                  // total attenuation cannot exceed max_db   
                  max_dB) * 
                  // Total attenuation in db * 2 to calculation value in 1/2 dB steps 
                  -2;                                           
}

void setVolume(int16_t newVolumeStep)
{
  if (appMode == APP_NORMAL_MODE || appMode == APP_BALANCE_MODE)
  {
    if (newVolumeStep < Settings.Input[RuntimeSettings.CurrentInput].MinVol)
      newVolumeStep = Settings.Input[RuntimeSettings.CurrentInput].MinVol;
    else if (newVolumeStep > Settings.Input[RuntimeSettings.CurrentInput].MaxVol)
      newVolumeStep = Settings.Input[RuntimeSettings.CurrentInput].MaxVol;

    // TO DO call Muses with balance logic
    if (!RuntimeSettings.Muted)
    {
      if (Settings.Input[RuntimeSettings.CurrentInput].Active != INPUT_HT_PASSTHROUGH)
        RuntimeSettings.CurrentVolume = newVolumeStep;
      else
        RuntimeSettings.CurrentVolume = Settings.Input[RuntimeSettings.CurrentInput].MaxVol; // Set to max volume
      RuntimeSettings.InputLastVol[RuntimeSettings.CurrentInput] = RuntimeSettings.CurrentVolume;

      int Attenuation = getAttenuation(Settings.VolumeSteps, RuntimeSettings.CurrentVolume, Settings.MinAttenuation, Settings.MaxAttenuation);
      debug(" Attenuation: "); debugln(Attenuation);

      muses.setVolume(Attenuation);

      if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] == 127 || RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] < 118 || RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] > 136) // Both channels same attenuation
        muses.setVolume(Attenuation);
      else if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] < 127) // Shift balance to the left channel by lowering the right channel - TO DO: seems like the channels is reversed in the Muses library??
        muses.setVolume(Attenuation + (127 - RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput]), Attenuation);
      else if (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] > 127) // Shift balance to the right channel by lowering the left channel - TO DO: seems like the channels is reversed in the Muses library??
        muses.setVolume(Attenuation, Attenuation + (RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] - 127));
    }
    if (appMode == APP_NORMAL_MODE)
      displayVolume();
  }
  notifyClients(getJSONCurrentVolume());
}

void mute()
{
  if (Settings.MuteLevel)
    muses.setVolume(getAttenuation(Settings.VolumeSteps, Settings.MuteLevel, Settings.MinAttenuation, Settings.MaxAttenuation));
  else
    muses.mute();
  RuntimeSettings.Muted = true;
}

void unmute()
{
  RuntimeSettings.Muted = false;
  setVolume(RuntimeSettings.CurrentVolume);
}

void displayVolume()
{
  if (Settings.DisplayVolume)
  {
    if (ScreenSaverIsOn)
      ScreenSaverOff();
    if (!RuntimeSettings.Muted)
    {
      // If show volume in steps
      if (Settings.DisplayVolume == 1)
      {
        if (Settings.VolumeSteps > 100)
        {
          oled.setCursor(17, 0);
          oled.print(F("Vol"));
          oled.print3x3Number(11, 1, RuntimeSettings.CurrentVolume, false); // Display volume from 000-999 with 3x3 digits
        }
        else
          oled.print4x4Number(11, RuntimeSettings.CurrentVolume); // Display volume from 00-99 with 4x4 digits
      }
      else // Show volume in -dB (-99.5 to 0)
      {
        oled.setCursor(17, 0);
        oled.print(F("-dB"));
        oled.print3x3Number(10, 1, (getAttenuation(Settings.VolumeSteps, RuntimeSettings.CurrentVolume, Settings.MinAttenuation, Settings.MaxAttenuation) / 2) * -10, true); // Display volume as -dB - RuntimeSettings.CurrentAttennuation are converted to -dB and multiplied by 10 to be able to show 0.5 dB steps
      }
    }
    else
      displayMute();
  }
}

// Clear previously displayed volume steps/-dB to indicate that mute is selected
void displayMute()
{
  if (ScreenSaverIsOn)
    ScreenSaverOff();
  for (int8_t i = 0; i < 4; i++)
  {
    oled.setCursor(10, i);
    for (int8_t x = 0; x < 10; x++)
      oled.write(32);
  }
}

boolean setInput(uint8_t NewInput)
{
  boolean result = false;
  if (Settings.Input[NewInput].Active != INPUT_INACTIVATED && NewInput >= 0 && NewInput <= 5 && appMode == APP_NORMAL_MODE)
  {
    if (!RuntimeSettings.Muted)
      mute();

    // Unselect currently selected input
    relayController.digitalWrite(RuntimeSettings.CurrentInput, LOW);

    // Save the currently selected input to enable switching between two inputs
    RuntimeSettings.PrevSelectedInput = RuntimeSettings.CurrentInput;

    // Select new input
    RuntimeSettings.CurrentInput = NewInput;
    relayController.digitalWrite(NewInput, HIGH);

    if (Settings.RecallSetLevel)
      RuntimeSettings.CurrentVolume = RuntimeSettings.InputLastVol[RuntimeSettings.CurrentInput];
    else if (RuntimeSettings.CurrentVolume > Settings.Input[RuntimeSettings.CurrentInput].MaxVol)
      RuntimeSettings.CurrentVolume = Settings.Input[RuntimeSettings.CurrentInput].MaxVol;
    else if (RuntimeSettings.CurrentVolume < Settings.Input[RuntimeSettings.CurrentInput].MinVol)
      RuntimeSettings.CurrentVolume = Settings.Input[RuntimeSettings.CurrentInput].MinVol;
    setVolume(RuntimeSettings.CurrentVolume);
    if (RuntimeSettings.Muted)
      unmute();

    displayInput();
    result = true;
  }

  notifyClients(getJSONCurrentInput());
  return result;
}

// Select the next active input (DOWN)
void setPrevInput()
{
  byte nextInput = (RuntimeSettings.CurrentInput == 0) ? 5 : RuntimeSettings.CurrentInput - 1;

  while (Settings.Input[nextInput].Active == INPUT_INACTIVATED)
  {
    nextInput = (nextInput == 0) ? 5 : nextInput - 1;
  }
  setInput(nextInput);
}

// Select the next active input (UP)
void setNextInput()
{
  byte nextInput = (RuntimeSettings.CurrentInput == 5) ? 0 : RuntimeSettings.CurrentInput + 1;
  while (Settings.Input[nextInput].Active == INPUT_INACTIVATED)
  {
    nextInput = (nextInput > 5) ? 0 : nextInput + 1;
  }
  setInput(nextInput);
}

// Display the name of the current input (but only if it has been chosen to be so by the user)
void displayInput()
{
  if (Settings.DisplaySelectedInput)
  {
    if (ScreenSaverIsOn)
      ScreenSaverOff();
    // Move Input display one line down if display of temperatures has been disabled
    if (!Settings.DisplayTemperature1 && !Settings.DisplayTemperature2)
      oled.setCursor(0, 1);
    else
      oled.setCursor(0, 0);
    oled.print(Settings.Input[RuntimeSettings.CurrentInput].Name);
  }
}

void displayTemperatures()
{
  if (ScreenSaverIsOn == false) // Only update display if screen saver is not on
  {

    if (Settings.DisplayTemperature1)
    {
      float Temp = getTemperature(NTC1_PIN);
      float MaxTemp;
      if (Settings.Trigger1Temp == 0)
        MaxTemp = 60; // TO DO: Is this the best default value?
      else
        MaxTemp = Settings.Trigger1Temp;
      displayTempDetails(Temp, MaxTemp, Settings.DisplayTemperature1, 1);
    }

    if (Settings.DisplayTemperature2)
    {
      float Temp = getTemperature(NTC2_PIN);
      float MaxTemp;
      if (Settings.Trigger2Temp == 0)
        MaxTemp = 60; // TO DO: Is this the best default value?
      else
        MaxTemp = Settings.Trigger2Temp;
      if (Settings.DisplayTemperature1)
        displayTempDetails(Temp, MaxTemp, Settings.DisplayTemperature1, 2);
      else
        displayTempDetails(Temp, MaxTemp, Settings.DisplayTemperature1, 1);
    }
  }
  mil_onRefreshTemperatureDisplay = millis();
}

void displayTempDetails(float Temp, uint8_t TriggerTemp, uint8_t DispTemp, uint8_t FirstOrSecond)
{
  byte Col;
  if (FirstOrSecond == 1)
    Col = 0;
  else
    Col = 5;
  oled.setCursor(Col, 3);
  if (Temp == 0)
  {
    oled.print(F("   "));
    if (DispTemp == 3)
    {
      oled.setCursor(Col, 2);
      oled.print(F("   "));
    }
  }
  else if (Temp > TriggerTemp)
  {
    oled.setCursor(Col, 3);
    oled.print(F("HIGH"));
    if (DispTemp == 3)
    {
      oled.setCursor(Col, 2);
      oled.print(F("TEMP"));
    }
  }
  else
  {
    if (DispTemp == 1 || DispTemp == 3)
    {
      oled.setCursor(Col, 3);
      oled.print(int(Temp));
      oled.write(128); // Degree symbol
      oled.print(" ");
    }
    if (DispTemp == 2 || DispTemp == 3)
    {
      if (DispTemp == 2)
        oled.setCursor(Col, 3);
      else
        oled.setCursor(Col, 2);

      // Map the range (0c ~ max temperature) to the range of the bar (0 to Number of characters to show the bar * Number of possible values per character )
      byte nb_columns = map(Temp, 0, TriggerTemp, 0, 4 * 5);

      for (byte i = 0; i < 4; ++i) // Number of characters to show the bar = 4
      {

        // Write character depending on number of columns remaining to display
        if (nb_columns == 0)
        { // Case empty
          oled.write(32);
        }
        else if (nb_columns >= 5) // Full box
        {
          oled.write(208); // Full box symbol
          nb_columns -= 5;
        }
        else // Partial box
        {
          oled.write(map(nb_columns, 1, 4, 212, 209)); // Map the remaining nb_columns (case between 1 and 4) to the corresponding character number symbols : 212 = 1 bar, 211 = 2 bars, 210 = 3 bars, 209 = 4 bars
          nb_columns = 0;
        }
      }
    }
  }
}

// Read analog with improved accuracy - see https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement/blob/main/ESP32_ADC_Read_Voltage_Accuracy_V2.ino
float readVoltage(byte ADC_Pin)
{
  float vref = 1100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  vref = adc_chars.vref;                                                                  // Obtain the device ADC reference voltage
  return (analogRead(ADC_Pin) / 4095.0) * 3.3 * (1100 / vref) * Settings.ADC_Calibration; // ESP by design reference voltage in mV
}

// Return measured temperature from 4.7K NTC connected to pinNmbr
float getTemperature(uint8_t pinNmbr)
{
  float Vin = 3.3;   // Input voltage 3.3V for ESP32
  float Vout = 0;    // Measured voltage
  float Rref = 2200; // Reference resistor's value in ohms
  float Rntc = 0;    // Measured resistance of NTC+
  float Temp;

  for (uint8_t i = 0; i < 8; i++)
    Vout = Vout + readVoltage(pinNmbr); // Read Vout on analog input pin (ESP32 can sense from 0-4095, 4095 is Vin)
  Vout = Vout / 8;

  Rntc = Rref * (1 / ((Vin / Vout) - 1)); // Formula to calculate the resistance of the NTC

  if (Rntc < 0)
    Temp = 0;
  else
    Temp = (-25.37 * log(Rntc)) + 239.43, 0.0; // Formula to calculate the temperature based on the resistance of the NTC - the formula is derived from the datasheet of the NTC

  if (Temp < 0)
    Temp = 0;
  else if (Temp > 99)
    Temp = 99;
  debug(" Voltage: ");
  debug(Vout);
  debug(" Resistance: ");
  debug(Rntc);
  debug("  Temp: ");
  debugln(Temp);
  return (Temp);
}

void loop()
{
  UIkey = getUserInput();

  switch (appMode)
  {
  case APP_NORMAL_MODE:
    if (millis() > mil_onRefreshTemperatureDisplay + TEMP_REFRESH_INTERVAL)
    {
      displayTemperatures();
      notifyClients(getJSONTempValues());
      if (((Settings.Trigger1Temp != 0) && (getTemperature(NTC1_PIN) >= Settings.Trigger1Temp)) || ((Settings.Trigger2Temp != 0) && (getTemperature(NTC2_PIN) >= Settings.Trigger2Temp)))
      {
        toStandbyMode();
      }
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
      // Turn volume up if we're not muted and we'll not exceed the maximum volume set for the currently selected input
      // TO DO: The checks for mute and MaxVol are done in setVolume so can be deleted here?
      if (!RuntimeSettings.Muted && (RuntimeSettings.CurrentVolume < Settings.Input[RuntimeSettings.CurrentInput].MaxVol))
        setVolume(RuntimeSettings.CurrentVolume + 1);
      break;
    case KEY_DOWN:
      // Turn volume down if we're not muted and we'll not get below the minimum volume set for the currently selected input
      // TO DO: The checks for mute and MinVol are done in setVolume so can be deleted here?
      if (!RuntimeSettings.Muted && (RuntimeSettings.CurrentVolume > Settings.Input[RuntimeSettings.CurrentInput].MinVol))
        setVolume(RuntimeSettings.CurrentVolume - 1);
      break;
    case KEY_LEFT:
    {
      setPrevInput();
      break;
    }
    case KEY_RIGHT:
    {
      setNextInput();
      break;
    }
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
      setInput(UIkey - KEY_1);
      break;
    case KEY_PREVIOUS:
      // Switch to previous selected input (to allow for A-B comparison)
      setInput(RuntimeSettings.PrevSelectedInput);
      break;
    case KEY_MUTE:
      // toggle mute
      if (RuntimeSettings.Muted)
        unmute();
      else
      {
        mute();
        displayMute();
      }
      break;
    case KEY_SELECT:
      // Set channel balance
      changeBalance();
      toAppNormalMode();
      break;
    }
    break;

  case APP_MENU_MODE:
  { // Brackets to avoid warning: "jump to case label [-fpermissive]"
    byte menuMode = Menu1.handleNavigation(getNavAction, refreshMenuDisplay);

    if (menuMode == MENU_EXIT)
    {
      // Back to APP_NORMAL_MODE
      toAppNormalMode();
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

    if (processingComplete == ABANDON)
    {
      // Back to APP_NORMAL_MODE
      toAppNormalMode();
      Menu1.reset();
    }
    else if (processingComplete == true)
    {
      appMode = APP_MENU_MODE;
      drawMenu();
    }
    break;
  }

  case APP_STANDBY_MODE:
  {
    // Do nothing if in APP_STANDBY_MODE - if the user presses KEY_ONOFF a restart is done by getUserInput(). By the way: you don't need an IR remote: a doubleclick on encoder_2 is also KEY_ONOFF

    // Send temperature notification via websocket while in standby mode
    if (millis() > mil_onRefreshTemperatureDisplay + (TEMP_REFRESH_INTERVAL_STANDBY))
    {
      notifyClients(getJSONTempValues());
      mil_onRefreshTemperatureDisplay = millis();
    }
    break;
  }
  }
}

void toAppNormalMode()
{
  oled.clear();
  displayInput();
  displayVolume();
  displayTemperatures();
  appMode = APP_NORMAL_MODE;
}

void toStandbyMode()
{
  appMode = APP_STANDBY_MODE;
  writeRuntimeSettingsToEEPROM();
  ScreenSaverOff(); // Disable screen saver
  oled.clear();
  oled.setCursor(0, 1);
  oled.print(F("Going to sleep!"));
  oled.setCursor(11, 3);
  oled.print(F("...zzzZZZ"));
  mute();
  setTrigger1Off();
  setTrigger2Off();
  if (Settings.ExtPowerRelayTrigger)
  {
    digitalWrite(POWER_RELAY_PIN, LOW);
  }
  last_KEY_ONOFF = millis();
  notifyClients(getJSONOnStandbyState());
  delay(3000);
  oled.lcdOff();
}

//----------------------------------------------------------------------
// Addition or removal of menu items in MenuData.h will require this method to be modified accordingly.
byte processMenuCommand(byte cmdId)
{
  byte complete = false; // set to true when menu command processing complete. Set to ABANDON_MENU if a return to APP_MODE_NORMAL must be forced

  if (UIkey == KEY_SELECT)
  {
    complete = true;
  }

  switch (cmdId)
  {
  case mnuCmdVOL_STEPS:
  {
    // TO DO: Why did we choose 179 as maximum number of steps? (see also mnuCmdMAX_ATT)
    if (editNumericValue(Settings.VolumeSteps, 1, 179, "Steps"))
    {
      // Update MaxVol for all inputs to VolumeSteps and set MinVol = 0 for all inputs.
      for (uint8_t i = 0; i < 6; i++)
      {
        Settings.Input[i].MaxVol = Settings.VolumeSteps;
        Settings.Input[i].MinVol = 0;
        RuntimeSettings.InputLastVol[i] = 0;
      }
      // Validate if changed MaxStartVolume > VolumeStepsMaxStartVolume - if so, change MaxStartVolume to VolumeSteps
      if (Settings.MaxStartVolume > Settings.VolumeSteps)
        Settings.MaxStartVolume = Settings.VolumeSteps;
      Settings.MuteLevel = 0;
      setVolume(0); // Turn the volume down to the minimum (just in case)
      writeSettingsToEEPROM();
    }
    complete = true;
    break;
  }
  case mnuCmdMIN_ATT:
    editNumericValue(Settings.MinAttenuation, 0, Settings.MaxAttenuation, "  -dB");
    setVolume(0); // Turn the volume down to the minimum (just in case)
    complete = true;
    break;
  case mnuCmdMAX_ATT:
    // TO DO: Why did we choose 90 dB as maximum attenuation? We can display up to 99.5 dB so it is probably just a decision (see also mnuCmdVOL_STEPS)
    editNumericValue(Settings.MaxAttenuation, Settings.MinAttenuation + 1, 90, "  -dB");
    setVolume(0); // Turn the volume down to the minimum (just in case)
    complete = true;
    break;
  case mnuCmdMAX_START_VOL:
    editNumericValue(Settings.MaxStartVolume, 0, Settings.VolumeSteps, " Step");
    complete = true;
    break;
  case mnuCmdMUTE_LVL:
    editNumericValue(Settings.MuteLevel, 0, Settings.VolumeSteps, " Step");
    complete = true;
    break;
  case mnuCmdSTORE_LVL:
    editOptionValue(Settings.RecallSetLevel, 2, "No", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT1_ACTIVE:
    if (RuntimeSettings.CurrentInput != 0) // If this input is selected then only allow to select "HT", "Yes". If not "HT", "Yes", "No" is allowed as options
      editOptionValue(Settings.Input[0].Active, 3, "HT", "Yes", "No", "");
    else
      editOptionValue(Settings.Input[0].Active, 2, "HT", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT1_NAME:
    editInputName(0);
    complete = true;
    break;
  case mnuCmdINPUT1_MAX_VOL:
    editNumericValue(Settings.Input[0].MaxVol, 0, Settings.VolumeSteps, " Step");
    complete = true;
    break;
  case mnuCmdINPUT1_MIN_VOL:
    editNumericValue(Settings.Input[0].MinVol, 0, Settings.Input[0].MaxVol, " Step");
    complete = true;
    break;
  case mnuCmdINPUT2_ACTIVE:
    if (RuntimeSettings.CurrentInput != 1) // If this input is selected then only allow to select "HT", "Yes". If not "HT", "Yes", "No" is allowed as options
      editOptionValue(Settings.Input[1].Active, 3, "HT", "Yes", "No", "");
    else
      editOptionValue(Settings.Input[1].Active, 2, "HT", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT2_NAME:
    editInputName(1);
    complete = true;
    break;
  case mnuCmdINPUT2_MAX_VOL:
    editNumericValue(Settings.Input[1].MaxVol, 0, Settings.VolumeSteps, " Step");
    complete = true;
    break;
  case mnuCmdINPUT2_MIN_VOL:
    editNumericValue(Settings.Input[1].MinVol, 0, Settings.Input[1].MaxVol, " Step");
    complete = true;
    break;
  case mnuCmdINPUT3_ACTIVE:
    if (RuntimeSettings.CurrentInput != 2) // If this input is selected then only allow to select "HT", "Yes". If not "HT", "Yes", "No" is allowed as options
      editOptionValue(Settings.Input[2].Active, 3, "HT", "Yes", "No", "");
    else
      editOptionValue(Settings.Input[2].Active, 2, "HT", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT3_NAME:
    editInputName(2);
    complete = true;
    break;
  case mnuCmdINPUT3_MAX_VOL:
    editNumericValue(Settings.Input[2].MaxVol, 0, Settings.VolumeSteps, " Step");
    complete = true;
    break;
  case mnuCmdINPUT3_MIN_VOL:
    editNumericValue(Settings.Input[2].MinVol, 0, Settings.Input[2].MaxVol, " Step");
    complete = true;
    break;
  case mnuCmdINPUT4_ACTIVE:
    if (RuntimeSettings.CurrentInput != 3) // If this input is selected then only allow to select "HT", "Yes". If not "HT", "Yes", "No" is allowed as options
      editOptionValue(Settings.Input[3].Active, 3, "HT", "Yes", "No", "");
    else
      editOptionValue(Settings.Input[3].Active, 2, "HT", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT4_NAME:
    editInputName(3);
    complete = true;
    break;
  case mnuCmdINPUT4_MAX_VOL:
    editNumericValue(Settings.Input[3].MaxVol, 0, Settings.VolumeSteps, " Step");
    complete = true;
    break;
  case mnuCmdINPUT4_MIN_VOL:
    editNumericValue(Settings.Input[3].MinVol, 0, Settings.Input[3].MaxVol, " Step");
    complete = true;
    break;
  case mnuCmdINPUT5_ACTIVE:
    if (RuntimeSettings.CurrentInput != 4) // If this input is selected then only allow to select "HT", "Yes". If not "HT", "Yes", "No" is allowed as options
      editOptionValue(Settings.Input[4].Active, 3, "HT", "Yes", "No", "");
    else
      editOptionValue(Settings.Input[4].Active, 2, "HT", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT5_NAME:
    editInputName(4);
    complete = true;
    break;
  case mnuCmdINPUT5_MAX_VOL:
    editNumericValue(Settings.Input[4].MaxVol, 0, Settings.VolumeSteps, " Step");
    complete = true;
    break;
  case mnuCmdINPUT5_MIN_VOL:
    editNumericValue(Settings.Input[4].MinVol, 0, Settings.Input[4].MaxVol, " Step");
    complete = true;
    break;
  case mnuCmdINPUT6_ACTIVE:
    if (RuntimeSettings.CurrentInput != 5) // If this input is selected then only allow to select "HT", "Yes". If not "HT", "Yes", "No" is allowed as options
      editOptionValue(Settings.Input[5].Active, 3, "HT", "Yes", "No", "");
    else
      editOptionValue(Settings.Input[5].Active, 2, "HT", "Yes", "", "");
    complete = true;
    break;
  case mnuCmdINPUT6_NAME:
    editInputName(5);
    complete = true;
    break;
  case mnuCmdINPUT6_MAX_VOL:
    editNumericValue(Settings.Input[5].MaxVol, 0, Settings.VolumeSteps, " Step");
    complete = true;
    break;
  case mnuCmdINPUT6_MIN_VOL:
    editNumericValue(Settings.Input[5].MinVol, 0, Settings.Input[5].MaxVol, " Step");
    complete = true;
    break;
  case mnuCmdIR_ONOFF:
    editIRCode(Settings.IR_ONOFF);
    complete = true;
    break;
  case mnuCmdIR_UP:
    editIRCode(Settings.IR_UP);
    complete = true;
    break;
  case mnuCmdIR_DOWN:
    editIRCode(Settings.IR_DOWN);
    complete = true;
    break;
  case mnuCmdIR_REPEAT:
    editIRCode(Settings.IR_REPEAT);
    complete = true;
    break;
  case mnuCmdIR_LEFT:
    editIRCode(Settings.IR_LEFT);
    complete = true;
    break;
  case mnuCmdIR_RIGHT:
    editIRCode(Settings.IR_RIGHT);
    complete = true;
    break;
  case mnuCmdIR_SELECT:
    editIRCode(Settings.IR_SELECT);
    complete = true;
    break;
  case mnuCmdIR_BACK:
    editIRCode(Settings.IR_BACK);
    complete = true;
    break;
  case mnuCmdIR_MUTE:
    editIRCode(Settings.IR_MUTE);
    complete = true;
    break;
  case mnuCmdIR_PREV:
    editIRCode(Settings.IR_PREVIOUS);
    complete = true;
    break;
  case mnuCmdIR_1:
    editIRCode(Settings.IR_1);
    complete = true;
    break;
  case mnuCmdIR_2:
    editIRCode(Settings.IR_2);
    complete = true;
    break;
  case mnuCmdIR_3:
    editIRCode(Settings.IR_3);
    complete = true;
    break;
  case mnuCmdIR_4:
    editIRCode(Settings.IR_4);
    complete = true;
    break;
  case mnuCmdIR_5:
    editIRCode(Settings.IR_5);
    complete = true;
    break;
  case mnuCmdIR_6:
    editIRCode(Settings.IR_6);
    complete = true;
    break;
  case mnuCmdTRIGGER1_ACTIVE:
    editOptionValue(Settings.Trigger1Active, 2, "Inactive", "Active", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER1_TYPE:
    editOptionValue(Settings.Trigger1Type, 2, "Moment.", "Latching", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER1_ON_DELAY:
    editNumericValue(Settings.Trigger1OnDelay, 0, 90, "Secs.");
    complete = true;
    break;
  case mnuCmdTRIGGER1_TEMP:
    editNumericValue(Settings.Trigger1Temp, 0, 90, "Deg C");
    complete = true;
    break;
  case mnuCmdTRIGGER2_ACTIVE:
    editOptionValue(Settings.Trigger2Active, 2, "Inactive", "Active", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER2_TYPE:
    editOptionValue(Settings.Trigger2Type, 2, "Moment.", "Latching", "", "");
    complete = true;
    break;
  case mnuCmdTRIGGER2_ON_DELAY:
    editNumericValue(Settings.Trigger2OnDelay, 0, 90, "Secs.");
    complete = true;
    break;
  case mnuCmdTRIGGER2_TEMP:
    editNumericValue(Settings.Trigger2Temp, 0, 90, "Deg C");
    complete = true;
    break;
  case mnuCmdTRIGGER_INACT_TIMER:
    editNumericValue(Settings.TriggerInactOffTimer, 0, 24, "Hours");
    complete = true;
    break;
  case mnuCmdDISP_SAVER_ACTIVE:
    editOptionValue(Settings.ScreenSaverActive, 2, "Off", "On", "", "");
    complete = true;
    break;
  case mnuCmdDISP_ON_LEVEL:
    editOptionValue(Settings.DisplayOnLevel, 4, "25%", "50%", "75%", "100%");
    oled.backlight((Settings.DisplayOnLevel + 1) * 64 - 1);
    complete = true;
    break;
  case mnuCmdDISP_DIM_LEVEL:
    editNumericValue(Settings.DisplayDimLevel, 0, 32, "     ");
    if (Settings.DisplayDimLevel != 0)
    {
      oled.backlight(Settings.DisplayDimLevel * 4 - 1);
      delay(2000);
      oled.backlight((Settings.DisplayOnLevel + 1) * 64 - 1);
    }
    complete = true;
    break;
  case mnuCmdDISP_DIM_TIMEOUT:
    editNumericValue(Settings.DisplayTimeout, 0, 90, "Secs.");
    complete = true;
    break;
  case mnuCmdDISP_VOL:
    editOptionValue(Settings.DisplayVolume, 3, "Hide", "Steps", "-dB", "");
    complete = true;
    break;
  case mnuCmdDISP_INPUT:
    editOptionValue(Settings.DisplaySelectedInput, 2, "Hide", "Show", "", "");
    complete = true;
    break;
  case mnuCmdDISP_TEMP1:
    editOptionValue(Settings.DisplayTemperature1, 4, "None", "Degrees", "Bar", "Both");
    complete = true;
    break;
  case mnuCmdDISP_TEMP2:
    editOptionValue(Settings.DisplayTemperature2, 4, "None", "Degrees", "Bar", "Both");
    complete = true;
    break;
  case mnuCmdABOUT:
    oled.clear();
    oled.setCursor(0, 0);
    oled.print(F("Firmware "));
    oled.print(VERSION);
    oled.setCursor(0, 1);
    oled.print(F("built by carsten"));
    oled.write(160);
    oled.setCursor(0, 2);
    oled.print(F("groenning.net &"));
    oled.setCursor(0, 3);
    oled.print(F("jan"));
    oled.write(160);
    oled.print(F("tofft.dk (c)2023"));
    delay(5000);
    complete = true;
    break;
  case mnuCmdSAVE_CUST:
    writeUserSettingsToEEPROM();
    oled.clear();
    oled.setCursor(0, 1);
    oled.print(F("Saved..."));
    delay(1000);
    complete = true;
    break;
  case mnuCmdLOAD_CUST:
    readUserSettingsFromEEPROM();
    writeSettingsToEEPROM();
    writeRuntimeSettingsToEEPROM();
    setTrigger1Off();
    setTrigger2Off();
    startUp();
    complete = ABANDON;
    break;
  case mnuCmdLOAD_DEFAULT:
    writeDefaultSettingsToEEPROM();
    setTrigger1Off();
    setTrigger2Off();
    startUp();
    complete = ABANDON;
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

//----------------------------------------------------------------------
// Show menu items based upon where the user has navigated to
// The menu consists of up to four lines: one line to show the name of the current menu and up to three lines of menu items.
void drawMenu()
{
  char strbuf[21]; // one line of lcd display
  char nameBuf[18];

  // Display the name of the menu
  oled.setCursor(0, 0);
  if (Menu1.currentMenuHasParent())
  {
    rpad(strbuf, Menu1.getParentItemName(nameBuf), ' ', 20);
    oled.print(strbuf);
  }
  else
    oled.print(F("Main menu           "));

  // Clear any previously displayed arrow
  for (int i = 1; i < 4; i++)
  {
    oled.setCursor(0, i);
    oled.print("  ");
  }

  // Display the name of the currently active menu item on the row set by menuIndex (+1 because row 0 is used to display the name of the menu)
  oled.setCursor(1, menuIndex + 1);
  oled.write(16); // Mark with an arrow that this is the menu item that will be activated if the user press select
  rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
  oled.print(strbuf);

  switch (menuIndex)
  {
  case 0:                                                            // The current menu item was displayed on row 0 - see if there is items to display on row 1 and 2
    if (Menu1.getCurrentItemIndex() + 1 <= Menu1.getMenuItemCount()) // Check if there is more menu items to display
    {
      Menu1.moveToNextItem();
      rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
      oled.setCursor(2, menuIndex + 2);
      oled.print(strbuf);
      if (Menu1.getCurrentItemIndex() + 2 <= Menu1.getMenuItemCount())
      {
        Menu1.moveToNextItem();
        rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
        oled.setCursor(2, menuIndex + 3);
        oled.print(strbuf);
        Menu1.moveToPreviousItem();
      }
      else
      {
        /* clear line at menuIndex + 3 */
        rpad(strbuf, " ", ' ', 17);
        oled.setCursor(2, menuIndex + 3);
        oled.print(strbuf);
      }
      Menu1.moveToPreviousItem();
    }
    else // No more items in the menu - make sure we clear up any previously displayed info from the display
    {
      /* clear line at menuIndex + 2 and menuIndex + 3*/
      rpad(strbuf, " ", ' ', 17);
      oled.setCursor(2, menuIndex + 2);
      oled.print(strbuf);
      oled.setCursor(2, menuIndex + 3);
      oled.print(strbuf);
    }

    break;
  case 1:                       // The current menu item was displayed on row 1 - display item on row 0 and see if there is an item to display on row 2
    Menu1.moveToPreviousItem(); // Move one item up in the menu to find the name of the item to be displayed one the line just before the current one
    rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
    oled.setCursor(2, menuIndex);
    oled.print(strbuf);
    Menu1.moveToNextItem(); // Move to next menu item to get back to current one
    if (Menu1.getCurrentItemIndex() + 1 <= Menu1.getMenuItemCount())
    {
      Menu1.moveToNextItem(); // Move to next menu item to find the name of the item after the current one
      rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
      oled.setCursor(2, menuIndex + 2);
      oled.print(strbuf);
      Menu1.moveToPreviousItem(); // Move to previous menu item to get back to current one
    }
    else // No more items in the menu - make sure we clear up any previously displayed info from the display
    {
      /* clear line at menuIndex + 2 */
      rpad(strbuf, " ", ' ', 17);
      oled.setCursor(2, menuIndex + 2);
      oled.print(strbuf);
    }
    break;
  case 2:                       // The current menu item was displayed on row 2 - display items on row 0 and 1
    Menu1.moveToPreviousItem(); // Move one item up in the menu to find the name of the item to be displayed one the line just before the current one
    rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
    oled.setCursor(2, menuIndex);
    oled.print(strbuf);
    Menu1.moveToPreviousItem(); // Once again we move one item up in the menu to find the name of the item to be displayed as the first one
    rpad(strbuf, Menu1.getCurrentItemName(nameBuf), ' ', 18);
    oled.setCursor(2, menuIndex - 1);
    oled.print(strbuf);
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
      oled.setCursor(1, menuIndex + 1);
      oled.write(32); // Delete the arrow previously set
      menuIndex--;
      // Redraw indication of what menu item is selected
      oled.setCursor(1, menuIndex + 1);
      oled.write(16); // Mark with an arrow that this is the menu item that will be activated if the user press select
    }
    break;
  case REFRESH_MOVE_NEXT: // user has navigated to next menu item.
    if (menuIndex == 2)
      drawMenu();
    else
    {
      oled.setCursor(1, menuIndex + 1);
      oled.write(32); // Delete the arrow previously set
      menuIndex++;
      // Redraw indication of what menu item is selected
      oled.setCursor(1, menuIndex + 1);
      oled.write(16); // Mark with an arrow that this is the menu item that will be activated if the user press select
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

// Change channel balance for current input
// The balance between channels can be shifted up to 4.5 dB in 0.5 dB steps
// 127 = no balance shift (values < 127 = shift balance to the left channel, values > 127 = shift balance to the right channel)
bool changeBalance()
{
  bool complete = false;
  bool result = false;
  byte OldValue = RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput];
  byte NewValue = RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput];

  appMode = APP_BALANCE_MODE;

  // Display the screen
  oled.clear();
  oled.print("Balance");
  displayBalance(NewValue);

  while (!complete)
  {
    mil_LastUserInput = millis(); // Prevent the screen saver to kick in while editing
    switch (getUserInput())
    {
    case KEY_UP:
      if (NewValue < 136)
      {
        NewValue++;
        RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] = NewValue;
        setVolume(RuntimeSettings.CurrentVolume);
        displayBalance(NewValue);
      }
      break;
    case KEY_DOWN:
      if (NewValue > 118)
      {
        NewValue--;
        RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] = NewValue;
        setVolume(RuntimeSettings.CurrentVolume);
        displayBalance(NewValue);
      }
      break;
    case KEY_SELECT:
      if (NewValue != OldValue)
      {
        writeRuntimeSettingsToEEPROM();
        result = true;
      }
      complete = true;
      break;
    case KEY_BACK:
      // Exit without saving new value
      RuntimeSettings.InputLastBal[RuntimeSettings.CurrentInput] = OldValue;
      // TO DO Set volume
      result = false;
      complete = true;
      break;
    default:
      break;
    }
  }
  return result;
}

void displayBalance(byte Value)
{
  oled.setCursor(1, 1);
  oled.print("---------=---------");
  oled.setCursor(Value - 117, 1);
  oled.write(31);
  oled.setCursor(1, 2);
  if (Value < 127)
    oled.printf("L         %.1f dB R", (127 - Value) * -0.5);
  else if (Value == 127)
    oled.print("L                 R");
  else if (Value > 127)
    oled.printf("L %.1f dB         R", (Value - 127) * -0.5);
}

//----------------------------------------------------------------------
// Editing of input names up to ten characters long
// A name can consist of upper or lower case characters, digits and space characters
// An empty name or a name only consisting of spaces are not allowed as it makes no sense
void editInputName(uint8_t InputNumber)
{
  bool complete = false;
  bool isUpperCase = true;     // show upper or lower case characters
  int arrowX = 1;              // text edit arrow start X position on selection line
  int arrowPointingUpDown = 0; // text edit arrow start direction: up == 0, down == 1
  String newInputName = "          ";
  // Display the screen
  oled.clear();
  oled.print("Input ");
  oled.print(InputNumber + 1);
  oled.setCursor(7, 0);
  oled.write(223); // Right arrow
  drawEditInputNameScreen(isUpperCase);

  oled.setCursor(arrowX, 2);
  oled.write(byte(26 + arrowPointingUpDown)); // 26 is arrow up, 27 is arrow down
  newInputName = Settings.Input[InputNumber].Name;
  newInputName.trim();
  oled.setCursor(9, 0);
  oled.print(newInputName);
  oled.setCursor(9 + newInputName.length(), 0);
  oled.BlinkingCursorOn();
  while (!complete)
  {
    mil_LastUserInput = millis(); // Prevent the screen saver to kick in while editing
    switch (byte UserInput = getUserInput())
    {
    case KEY_UP:
    case KEY_DOWN:
      oled.BlinkingCursorOff();
      // Clear current arrow
      oled.setCursor(arrowX, 2);
      oled.write(' ');

      // Decide if position or direction of arrow must be changed
      if (arrowPointingUpDown == 0 && UserInput == KEY_UP)        // The arrow points up and the user input is "turn to the right"
        arrowPointingUpDown = 1;                                  // Set the arrow to point down but don't change position of ArrowX
      else if (arrowPointingUpDown == 0 && UserInput == KEY_DOWN) // The arrow points up and the user input is "turn to the left"
      {
        if (arrowX > 0)
          arrowX--; // Move arrow one postion to the left
      }
      else if (arrowPointingUpDown == 1 && UserInput == KEY_UP) // The arrow points down and the user input is "turn to the right"
      {
        if (arrowX < 19)
          arrowX++; // Move arrow one postion to the right
      }
      else if (arrowPointingUpDown == 1 && UserInput == KEY_DOWN) // The arrow points down and the user input is "turn to the left"
        arrowPointingUpDown = 0;                                  // Set the arrow to point up but don't change position of ArrowX

      // Display arrow
      oled.setCursor(arrowX, 2);
      if (arrowPointingUpDown == 1) // if arrow == 1, then print arrow that points down; if arrow == 0, then print arrow that points up
        oled.write(27);
      else
        oled.write(26);
      oled.setCursor(9 + newInputName.length(), 0);
      oled.BlinkingCursorOn();
      break;
    case KEY_SELECT:
      oled.BlinkingCursorOff();
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
            oled.setCursor(9 + newInputName.length() - 1, 0);
            oled.print(" "); // Print to clear the deleted character on the display
            newInputName = newInputName.substring(0, newInputName.length() - 1);
          }
        }
        else if (arrowX == 19) // Done editing (the Enter icon has been selected)
        {
          newInputName.trim();
          if (newInputName == "") // If no characters in new name then reset to original name
            newInputName = Settings.Input[InputNumber].Name;
          else
          {
            // Save new name to Settings
            for (uint8_t i = 0; i < newInputName.length(); i++)
              Settings.Input[InputNumber].Name[i] = newInputName.charAt(i);
            // Pad Name with spaces - makes it easier to display
            for (uint8_t i = newInputName.length(); i < 10; i++)
              Settings.Input[InputNumber].Name[i] = ' ';
            Settings.Input[InputNumber].Name[10] = '\0';
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
        oled.setCursor(9, 0);
        oled.print(newInputName);
        oled.BlinkingCursorOn();
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
  oled.BlinkingCursorOff();
}

void drawEditInputNameScreen(bool isUpperCase)
{
  oled.setCursor(0, 1);
  oled.write(byte(196)); // Print underscore to indicate Space
  if (isUpperCase)
  {
    for (int i = 65; i < 84; i++) // Print A-S
      oled.write(i);
    oled.setCursor(0, 3);
    for (int i = 84; i < 91; i++) // Print T-Z
      oled.write(i);
  }
  else
  {
    for (int i = 97; i < 116; i++) // Print a-s
      oled.write(i);
    oled.setCursor(0, 3);
    for (int i = 116; i < 123; i++) // Print t-z
      oled.write(i);
  }
  for (int i = 48; i < 58; i++) // Print 0-9
    oled.write(i);
  if (isUpperCase)
    oled.write(19);
  else
    oled.write(18);
  oled.write(225); // "Backspace" icon
  oled.write(28);  // "Enter" icon
}

bool editNumericValue(byte &Value, byte MinValue, byte MaxValue, const char Unit[5])
{
  bool complete = false;
  bool result = false;
  char nameBuf[11];

  byte NewValue = Value;

  // Display the screen
  oled.clear();
  oled.print(Menu1.getCurrentItemName(nameBuf));
  oled.setCursor(0, 2);
  oled.print(F("Min. "));
  oled.print(MinValue);
  oled.setCursor(0, 3);
  oled.print(F("Max. "));
  oled.print(MaxValue);
  oled.setCursor(15, 0);
  oled.print(Unit);

  oled.print3x3Number(11, 1, NewValue, false); // Display number from 000-999 with 3x3 digits

  while (!complete)
  {
    mil_LastUserInput = millis(); // Prevent the screen saver to kick in while editing
    switch (getUserInput())
    {
    case KEY_UP:
      if (NewValue < MaxValue)
      {
        NewValue++;
        oled.print3x3Number(11, 1, NewValue, false); // Display number from 000-999 with 3x3 digits
      }
      break;
    case KEY_DOWN:
      if (NewValue > MinValue)
      {
        NewValue--;
        oled.print3x3Number(11, 1, NewValue, false); // Display number from 000-999 with 3x3 digits
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
  oled.clear();
  oled.print(Menu1.getCurrentItemName(nameBuf));
  if (NumOptions < 3)
  {
    oled.setCursor(1, 2);
    oled.print(Option1);
    oled.setCursor(11, 2);
    oled.print(Option2);
  }
  else
  {
    oled.setCursor(1, 2);
    oled.print(Option1);
    oled.setCursor(11, 2);
    oled.print(Option2);
    oled.setCursor(1, 3);
    oled.print(Option3);
    oled.setCursor(11, 3);
    oled.print(Option4);
  }

  oled.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
  oled.write(16);

  while (!complete)
  {
    mil_LastUserInput = millis(); // Prevent the screen saver to kick in while editing
    switch (getUserInput())
    {
    case KEY_UP:
      oled.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
      oled.print(" ");
      if (NewValue < NumOptions - 1)
        NewValue++;
      else
        NewValue = 0;
      oled.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
      oled.write(16);
      break;
    case KEY_DOWN:
      oled.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
      oled.print(" ");
      if (NewValue == 0)
        NewValue = NumOptions - 1;
      else
        NewValue--;
      oled.setCursor((NewValue % 2) * 10, (NewValue / 2) + 2);
      oled.write(16);
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

bool editIRCode(IRMP_DATA &Value)
{
  bool complete = false;
  bool result = false;
  char nameBuf[11];

  IRMP_DATA NewValue, OldValue;
  NewValue.address = 0;
  NewValue.command = 0;

  // Display the screen
  oled.clear();
  oled.print(F("IR key "));
  oled.print(Menu1.getCurrentItemName(nameBuf));

  oled.setCursor(0, 1);
  oled.print(F("Current:"));
  oled.setCursor(0, 2);
  oled.print(Value.address, HEX);
  oled.setCursor(0, 3);
  oled.print(Value.command, HEX);
  oled.setCursor(10, 1);
  oled.print(F("New:"));
  oled.setCursor(10, 2);
  oled.print(NewValue.address, HEX);
  oled.setCursor(10, 3);
  oled.print(NewValue.command, HEX);

  // As we don't want to react to received IR code while learning new code we temporaryly disable the current code in the Settings (we save a copy in OldValue)
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
    if (irmp_get_data(&irmp_data))
    {
      // Get the new data from the remote
      NewValue.address = irmp_data.address;
      NewValue.command = irmp_data.command;
      oled.setCursor(10, 2);
      oled.print(F("          "));
      oled.setCursor(10, 2);
      oled.print(NewValue.address, HEX);
      oled.setCursor(10, 3);
      oled.print(F("          "));
      oled.setCursor(10, 3);
      oled.print(NewValue.command, HEX);
    }
  }
  return result;
}

// Loads default settings into Settings and RuntimeSettings - this is only done when the EEPROM does not contain valid settings or when reset is chosen by user in the menu
void setSettingsToDefault()
{
  strcpy(Settings.ssid, "                                ");
  strcpy(Settings.pass, "                                ");
  strcpy(Settings.ip, "               ");
  strcpy(Settings.gateway, "               ");
  Settings.ExtPowerRelayTrigger = true;
  Settings.ADC_Calibration = ADC_CALIBRATION;
  Settings.VolumeSteps = 60;
  Settings.MinAttenuation = 0;
  Settings.MaxAttenuation = 60;
  Settings.MaxStartVolume = Settings.VolumeSteps;
  Settings.MuteLevel = 0;
  Settings.RecallSetLevel = true;
  Settings.IR_UP.address = 0x2;
  Settings.IR_UP.command = 0xA;
  Settings.IR_DOWN.address = 0x2;
  Settings.IR_DOWN.command = 0xB;
  Settings.IR_REPEAT.address = 0x00;
  Settings.IR_REPEAT.command = 0x00;
  Settings.IR_LEFT.address = 0x2;
  Settings.IR_LEFT.command = 0x1D;
  Settings.IR_RIGHT.address = 0x2;
  Settings.IR_RIGHT.command = 0x1B;
  Settings.IR_SELECT.address = 0x2;
  Settings.IR_SELECT.command = 0x19;
  Settings.IR_BACK.address = 0x2;
  Settings.IR_BACK.command = 0x1F;
  Settings.IR_MUTE.address = 0x2;
  Settings.IR_MUTE.command = 0x1C;
  Settings.IR_PREVIOUS.address = 0x00;
  Settings.IR_PREVIOUS.command = 0x00;
  Settings.IR_ONOFF.address = 0x2;
  Settings.IR_ONOFF.command = 0xF;
  Settings.IR_1.address = 0x2;
  Settings.IR_1.command = 0x0;
  Settings.IR_2.address = 0x2;
  Settings.IR_2.command = 0x1;
  Settings.IR_3.address = 0x2;
  Settings.IR_3.command = 0x2;
  Settings.IR_4.address = 0x2;
  Settings.IR_4.command = 0x3;
  Settings.IR_5.address = 0x2;
  Settings.IR_5.command = 0x4;
  Settings.IR_6.address = 0x2;
  Settings.IR_6.command = 0x5;
  Settings.Input[0].Active = INPUT_NORMAL;
  strcpy(Settings.Input[0].Name, "Input 1   ");
  Settings.Input[0].MaxVol = Settings.VolumeSteps;
  Settings.Input[0].MinVol = 0;
  Settings.Input[1].Active = INPUT_NORMAL;
  strcpy(Settings.Input[1].Name, "Input 2   ");
  Settings.Input[1].MaxVol = Settings.VolumeSteps;
  Settings.Input[1].MinVol = 0;
  Settings.Input[2].Active = INPUT_NORMAL;
  strcpy(Settings.Input[2].Name, "Input 3   ");
  Settings.Input[2].MaxVol = Settings.VolumeSteps;
  Settings.Input[2].MinVol = 0;
  Settings.Input[3].Active = INPUT_NORMAL;
  strcpy(Settings.Input[3].Name, "Input 4   ");
  Settings.Input[3].MaxVol = Settings.VolumeSteps;
  Settings.Input[3].MinVol = 0;
  Settings.Input[4].Active = INPUT_NORMAL;
  strcpy(Settings.Input[4].Name, "Input 5   ");
  Settings.Input[4].MaxVol = Settings.VolumeSteps;
  Settings.Input[4].MinVol = 0;
  Settings.Input[5].Active = INPUT_NORMAL;
  strcpy(Settings.Input[5].Name, "Input 6   ");
  Settings.Input[5].MaxVol = Settings.VolumeSteps;
  Settings.Input[5].MinVol = 0;
  Settings.Trigger1Active = 1;
  Settings.Trigger1Type = 0;
  Settings.Trigger1OnDelay = 0;
  Settings.Trigger1Temp = 0;
  Settings.Trigger2Active = 1;
  Settings.Trigger2Type = 0;
  Settings.Trigger2OnDelay = 0;
  Settings.Trigger2Temp = 0;
  Settings.TriggerInactOffTimer = 0;
  Settings.ScreenSaverActive = true;
  Settings.DisplayOnLevel = 3;
  Settings.DisplayDimLevel = 0;
  Settings.DisplayTimeout = 30;
  Settings.DisplayVolume = 1;
  Settings.DisplaySelectedInput = true;
  Settings.DisplayTemperature1 = 3;
  Settings.DisplayTemperature2 = 3;
  Settings.Version = VERSION;

  RuntimeSettings.CurrentInput = 0;
  RuntimeSettings.CurrentVolume = 0;
  RuntimeSettings.Muted = 0;
  RuntimeSettings.InputLastVol[0] = 0;
  RuntimeSettings.InputLastVol[1] = 0;
  RuntimeSettings.InputLastVol[2] = 0;
  RuntimeSettings.InputLastVol[3] = 0;
  RuntimeSettings.InputLastVol[4] = 0;
  RuntimeSettings.InputLastVol[5] = 0;
  RuntimeSettings.InputLastBal[0] = 127; // 127 = no balance shift (values < 127 = shift balance to the left channel, values > 127 = shift balance to the right channel)
  RuntimeSettings.InputLastBal[1] = 127;
  RuntimeSettings.InputLastBal[2] = 127;
  RuntimeSettings.InputLastBal[3] = 127;
  RuntimeSettings.InputLastBal[4] = 127;
  RuntimeSettings.InputLastBal[5] = 127;
  RuntimeSettings.PrevSelectedInput = 0;
  RuntimeSettings.Version = VERSION;
}

// Write Settings to EEPROM
void writeSettingsToEEPROM()
{
  // Write the settings to the EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.write(0, Settings.data, sizeof(Settings));
}

// Read Settings from EEPROM
void readSettingsFromEEPROM()
{
  // Read settings from EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.read(0, Settings.data, sizeof(Settings));
}

// Write Default Settings and RuntimeSettings to EEPROM - called if the EEPROM data is not valid or if the user chooses to reset all settings to default value
void writeDefaultSettingsToEEPROM()
{
  // Read default settings into Settings
  setSettingsToDefault();
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
  eeprom.write(sizeof(Settings) + 1, RuntimeSettings.data, sizeof(RuntimeSettings));
}

// Read the last runtime settings from EEPROM
void readRuntimeSettingsFromEEPROM()
{
  // Read the settings from the EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.read(sizeof(Settings) + 1, RuntimeSettings.data, sizeof(RuntimeSettings));
}

// Read the user defined settings from EEPROM
void readUserSettingsFromEEPROM()
{
  // Read the settings from the EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.read(sizeof(Settings) + sizeof(RuntimeSettings) + 1, Settings.data, sizeof(Settings));
}

// Read the user defined settings from EEPROM
void writeUserSettingsToEEPROM()
{
  // Write the user settings to the EEPROM
  eeprom.begin(extEEPROM::twiClock400kHz);
  eeprom.write(sizeof(Settings) + sizeof(RuntimeSettings) + 1, Settings.data, sizeof(Settings));
}

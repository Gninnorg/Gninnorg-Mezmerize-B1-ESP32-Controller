/*
**
** Input controller for MezmerizeB1Buffer
**
** Copyright (c) 2020 Carsten Grønning, Jan Abkjer Tofft
**
** Version 0.1
**                     
*/

#include "Adafruit_MCP23008.h"

InputController::InputController(){}
InputController::~InputController(){}

Adafruit_MCP23008 mcp;


#define INPUTCOUNT 6    //** number of inputs, 0 to 6. If 0, comment out the next line.
char* inputName[INPUTCOUNT] = { "INPUT 1", "INPUT 2", "INPUT 3", "INPUT 4", "INPUT 5", "INPUT 6" }; //** each name maximum 9 characters. There must be exactly INPUTCOUNT names in the list.


void setInput() {
  bool wasMuted = isMuted;
  if (millis() - mil_onInput > TIME_RELAYLATCH) {
    lcd.setCursor(1, ROW_IN);
    lcd_print(inputName[chan_in], (LCDCOLUMNS / 2) - 1);

    for (byte i = 0; i < INPUTCOUNT; i++)
      mcp.digitalWrite(relayMap[i], (chan_in == i));

    mil_onInput = millis();
  }
  if (isMuted && !wasMuted) {
    if (ENABLEHTPASSTHROUGH && chan_in == HTPASSTHROUGHINPUT) {
      setVolume(HTPASSTHROUGHVOLUME); // Volume is fixed on the selected input
    }
    else {
      setVolume(volume);
    }
  }
}
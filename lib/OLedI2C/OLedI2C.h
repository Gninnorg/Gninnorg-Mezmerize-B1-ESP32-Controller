/*
Library to encapsulate use of the OLED 2004 from WIDE.HK
The display is based on the SSD1311.
The display has 20 characters and 4 lines.

The library is based on the sample code downloaded fra WIDE.HK
and the work of Phil Grant, 2013, www.gadjet.co.uk
Scrolling coded contributed by Nathan Chantrell http://nathan.chantrell.net/

Adapted and expanded by Jan Tofft, jan@tofft.dk, 2017
*/
#ifndef OLedI2C_h
#define OLedI2C_h	
#include "Arduino.h"
#include "Wire.h"

class OLedI2C {
public:
		OLedI2C();
		~OLedI2C();
		void begin();
		void sendCommand(unsigned char command);
		void sendString(const char *String, uint8_t col, uint8_t row);
		void print(const char *String);
    	void print(byte value);
		void sendFloat(float digit, uint8_t dec, uint8_t nad, uint8_t col, uint8_t row);
		void sendData(unsigned char data);
		void write(unsigned char data);
		void createChar(uint8_t, uint8_t[]);
		void clear();
		void setCursor(uint8_t, uint8_t); // Column, Row
		void scrollString(char* message, byte row, unsigned int time); //written by Nathan Chantrell http://nathan.chantrell.net/
		void lcdOff();
		void lcdOn();
		void FadeOut();
		void FadeOutCancel();
		void PowerDown();
		void PowerUp();
		void EnterSleepMode();
		void ExitingSleepMode();
		void backlight(unsigned char contrast); // contrast should be the hex value between 0x00 and 0xFF
};
#endif

/*
Library to encapsulate use of the OLED 2004 from WIDE.HK
The display is based on the SSD1311.
The display has 20 characters and 4 lines.

The library is based on the sample code downloaded fra WIDE.HK
and the work of Phil Grant, 2013, www.gadjet.co.uk

Adapted and expanded by Jan Tofft, jan@tofft.dk, 2017
Inheritance of Print functions added, jan@tofft.dk, 2020
*/
#ifndef OLedI2C_h
#define OLedI2C_h
#include "Arduino.h"
#include "Print.h"
#include <stddef.h>
#include <stdint.h>
#include "Wire.h"

class OLedI2C : public Print
{
public:
	OLedI2C();
	~OLedI2C();
	void begin();
	void sendCommand(uint8_t command);
	void sendData(uint8_t data);
	void createChar(uint8_t, uint8_t[]);
	void clear();
	void setCursor(uint8_t, uint8_t); // Column, Row
	void lcdOff();
	void lcdOn();
	void FadeOut();
	void FadeOutCancel();
	void PowerDown();
	void PowerUp();
	void EnterSleepMode();
	void ExitingSleepMode();
	void backlight(uint8_t contrast); // contrast should be the hex value between 0x00 and 0xFF
	void printTwoNumber(uint8_t column, uint8_t number); // prints large number
	void defineCustomChar();


	// support of Print class
	virtual size_t write(uint8_t ch);
	using Print::write;
};
#endif

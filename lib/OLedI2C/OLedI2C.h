/*
Library to encapsulate use of the OLED 2004 from WIDE.HK
The display is based on the SSD1311.
The display has 20 characters and 4 lines.

The library is based on the sample code downloaded fra WIDE.HK
and the work of Phil Grant, 2013, www.gadjet.co.uk

Adapted and expanded by Jan Tofft, jan@tofft.dk, 2017
Inheritance of Print functions added, jan@tofft.dk, 2020
Possibility to flip the display added (search for "Set Entry Mode (invert)" in OLedI2C.cpp), jan@tofft.dk, 2020
BlinkingCursorOn/Off added, jan@tofft.dk, 2020
*/
#ifndef OLedI2C_h
#define OLedI2C_h
#include "Arduino.h"
#include "Print.h"
#include <stddef.h>
#include <stdint.h>
#include "Wire.h"
#include <stdarg.h>

#define LCD_ROWS  4
#define LCD_COLS  20

// Apply right padding to string.
extern char *rpad (char *dest, const char *str, char chr = ' ', unsigned char width = LCD_COLS);

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
	void BlinkingCursorOn();
    void BlinkingCursorOff();
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

//	void print3x3Number(uint8_t column, uint8_t row, uint16_t number, uint8_t digits, bool decimalPoint); // prints large number 3x3 char per digit. The number of digits to be displayed can be set to 1, 2 or 3
//	void defineCustomChar3x3();

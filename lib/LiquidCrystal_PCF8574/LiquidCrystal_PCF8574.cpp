/// \file LiquidCrystal_PCF8574.cpp
/// \brief LiquidCrystal library with PCF8574 I2C adapter.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2019 by Matthias Hertel.
///
/// ChangeLog see: LiquidCrystal_PCF8574.h

#include "LiquidCrystal_PCF8574.h"

#include <Wire.h>

/// Definitions on how the PCF8574 is connected to the LCD

/// These are Bit-Masks for the special signals and background light
#define PCF_RS 0x01
#define PCF_RW 0x02
#define PCF_EN 0x04
#define PCF_BACKLIGHT 0x08
// the 0xF0 bits are used for 4-bit data to the display.

// a nibble is a half Byte

LiquidCrystal_PCF8574::LiquidCrystal_PCF8574(int i2cAddr)
{
  _i2cAddr = i2cAddr;
  _backlight = 0;

  _entrymode = 0x02; // like Initializing by Internal Reset Circuit
  _displaycontrol = 0x04;
} // LiquidCrystal_PCF8574

void LiquidCrystal_PCF8574::begin(int cols, int lines)
{
  // _cols = cols ignored !
  _lines = lines;

  int functionFlags = 0;

  if (lines > 1)
  {
    functionFlags |= 0x08;
  }

  // initializing the display
  Wire.begin();
  _write2Wire(0x00, LOW, false);
  delayMicroseconds(50000);

  // after reset the mode is this
  _displaycontrol = 0x04;
  _entrymode = 0x02;

  // sequence to reset. see "Initializing by Instruction" in datatsheet
  _sendNibble(0x03);
  delayMicroseconds(4500);
  _sendNibble(0x03);
  delayMicroseconds(200);
  _sendNibble(0x03);
  delayMicroseconds(200);
  _sendNibble(0x02); // finally, set to 4-bit interface

  // Instruction: Function set = 0x20
  _send(0x20 | functionFlags);

  display();
  clear();
  leftToRight();
} // begin()

void LiquidCrystal_PCF8574::clear()
{
  // Instruction: Clear display = 0x01
  _send(0x01);
  delayMicroseconds(1600); // this command takes 1.5ms!
} // clear()

void LiquidCrystal_PCF8574::init()
{
  clear();
} // init()

void LiquidCrystal_PCF8574::home()
{
  // Instruction: Return home = 0x02
  _send(0x02);
  delayMicroseconds(1600); // this command takes 1.5ms!
} // home()

/// Set the cursor to a new position.
void LiquidCrystal_PCF8574::setCursor(int col, int row)
{
  int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  // Instruction: Set DDRAM address = 0x80
  _send(0x80 | (row_offsets[row] + col));
} // setCursor()

// Turn the display on/off (quickly)
void LiquidCrystal_PCF8574::noDisplay()
{
  // Instruction: Display on/off control = 0x08
  _displaycontrol &= ~0x04; // display
  _send(0x08 | _displaycontrol);
} // noDisplay()

void LiquidCrystal_PCF8574::display()
{
  // Instruction: Display on/off control = 0x08
  _displaycontrol |= 0x04; // display
  _send(0x08 | _displaycontrol);
} // display()

// Turns the underline cursor on/off
void LiquidCrystal_PCF8574::cursor()
{
  // Instruction: Display on/off control = 0x08
  _displaycontrol |= 0x02; // cursor
  _send(0x08 | _displaycontrol);
} // cursor()

void LiquidCrystal_PCF8574::noCursor()
{
  // Instruction: Display on/off control = 0x08
  _displaycontrol &= ~0x02; // cursor
  _send(0x08 | _displaycontrol);
} // noCursor()

// Turn on and off the blinking cursor
void LiquidCrystal_PCF8574::blink()
{
  // Instruction: Display on/off control = 0x08
  _displaycontrol |= 0x01; // blink
  _send(0x08 | _displaycontrol);
} // blink()

void LiquidCrystal_PCF8574::noBlink()
{
  // Instruction: Display on/off control = 0x08
  _displaycontrol &= ~0x01; // blink
  _send(0x08 | _displaycontrol);
} // noBlink()

// These commands scroll the display without changing the RAM
void LiquidCrystal_PCF8574::scrollDisplayLeft(void)
{
  // Instruction: Cursor or display shift = 0x10
  // shift: 0x08, left: 0x00
  _send(0x10 | 0x08 | 0x00);
} // scrollDisplayLeft()

void LiquidCrystal_PCF8574::scrollDisplayRight(void)
{
  // Instruction: Cursor or display shift = 0x10
  // shift: 0x08, right: 0x04
  _send(0x10 | 0x08 | 0x04);
} // scrollDisplayRight()

// == controlling the entrymode

// This is for text that flows Left to Right
void LiquidCrystal_PCF8574::leftToRight(void)
{
  // Instruction: Entry mode set, set increment/decrement =0x02
  _entrymode |= 0x02;
  _send(0x04 | _entrymode);
} // leftToRight()

// This is for text that flows Right to Left
void LiquidCrystal_PCF8574::rightToLeft(void)
{
  // Instruction: Entry mode set, clear increment/decrement =0x02
  _entrymode &= ~0x02;
  _send(0x04 | _entrymode);
} // rightToLeft()

// This will 'right justify' text from the cursor
void LiquidCrystal_PCF8574::autoscroll(void)
{
  // Instruction: Entry mode set, set shift S=0x01
  _entrymode |= 0x01;
  _send(0x04 | _entrymode);
} // autoscroll()

// This will 'left justify' text from the cursor
void LiquidCrystal_PCF8574::noAutoscroll(void)
{
  // Instruction: Entry mode set, clear shift S=0x01
  _entrymode &= ~0x01;
  _send(0x04 | _entrymode);
} // noAutoscroll()

/// Setting the brightness of the background display light.
/// The backlight can be switched on and off.
/// The current brightness is stored in the private _backlight variable to have it available for further data transfers.
void LiquidCrystal_PCF8574::setBacklight(int brightness)
{
  _backlight = brightness;
  // send no data but set the background-pin right;
  _write2Wire(0x00, true, false);
} // setBacklight()

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystal_PCF8574::createChar(uint8_t location, uint8_t charmap[])
{
  location &= 0x7; // we only have 8 locations 0-7
  // Set CGRAM address
  _send(0x40 | (location << 3));
  for (int i = 0; i < 8; i++)
  {
    write(charmap[i]);
  }
} // createChar()

/* The write function is needed for derivation from the Print class. */
inline size_t LiquidCrystal_PCF8574::write(uint8_t ch)
{
  _send(ch, true);
  return 1; // assume sucess
} // write()

// write either command or data
void LiquidCrystal_PCF8574::_send(int value, bool isData)
{
  // write high 4 bits
  _sendNibble((value >> 4 & 0x0F), isData);
  // write low 4 bits
  _sendNibble((value & 0x0F), isData);
} // _send()

// write a nibble / halfByte with handshake
void LiquidCrystal_PCF8574::_sendNibble(int halfByte, bool isData)
{
  _write2Wire(halfByte, isData, true);
  delayMicroseconds(1); // enable pulse must be >450ns
  _write2Wire(halfByte, isData, false);
  delayMicroseconds(37); // commands need > 37us to settle
} // _sendNibble

// private function to change the PCF8674 pins to the given value
// Note:
// you may change this function what the display is attached to the PCF8574 in a different wiring.
void LiquidCrystal_PCF8574::_write2Wire(int halfByte, bool isData, bool enable)
{
  // map the given values to the hardware of the I2C schema
  int i2cData = halfByte << 4;
  if (isData)
    i2cData |= PCF_RS;
  // PCF_RW is never used.
  if (enable)
    i2cData |= PCF_EN;
  if (_backlight > 0)
    i2cData |= PCF_BACKLIGHT;

  Wire.beginTransmission(_i2cAddr);
  Wire.write(i2cData);
  Wire.endTransmission();
} // write2Wire

// Functions for printing two 4x4 digits. Works from 00-99
void LiquidCrystal_PCF8574::printTwoNumber(uint8_t column, uint8_t number)
{
  uint8_t firstdigit = (number / 10) * 4;
  uint8_t seconddigit = (number % 10) * 4;

  //                    0                1                2                3                4                5                6                7                8                9
  const uint8_t bn1[] = {  5,  2,  2,  1,  32,  5, 31, 32,   5,  2,  2,  1,   2,  2,  2,  1,  31, 32, 32, 31,  31,  2,  2,  2,   5,  2,  2,  2,   2,  2,  2, 31,   5,  2,  2,  1,   5,  2,  2,  1 };
  const uint8_t bn2[] = { 31, 32, 32, 31,  32, 32, 31, 32,   0,  3,  3,  7,  32,  3,  3, 31,   4,  3,  3, 31,   4,  3,  3,  6,  31,  3,  3,  6,  32, 32,  0,  7,  31,  3,  3, 31,   4,  3,  3, 31 };
  const uint8_t bn3[] = { 31, 32, 32, 31,  32, 32, 31, 32,  31, 32, 32, 32,  32, 32, 32, 31,  32, 32, 32, 31,  32, 32, 32, 31,  31, 32, 32, 31,  32, 32, 31, 32,  31, 32, 32, 31,  32, 32, 32, 31 };
  const uint8_t bn4[] = {  4,  3,  3,  7,  32,  3, 31,  3,   4,  3,  3,  3,   4,  3,  3,  7,  32, 32, 32, 31,   4,  3,  3,  7,   4,  3,  3,  7,  32, 32, 31, 32,   4,  3,  3,  7,   4,  3,  3,  7 };

  setCursor(column, 0);
  _send(bn1[firstdigit], true);
  _send(bn1[firstdigit + 1], true);
  _send(bn1[firstdigit + 2], true);
  _send(bn1[firstdigit + 3], true);
  _send(32, true); // Blank
  _send(bn1[seconddigit], true);
  _send(bn1[seconddigit + 1], true);
  _send(bn1[seconddigit + 2], true);
  _send(bn1[seconddigit + 3], true);

  setCursor(column, 1);
  _send(bn2[firstdigit], true);
  _send(bn2[firstdigit + 1], true);
  _send(bn2[firstdigit + 2], true);
  _send(bn2[firstdigit + 3], true);
  _send(32, true); // Blank
  _send(bn2[seconddigit], true);
  _send(bn2[seconddigit + 1], true);
  _send(bn2[seconddigit + 2], true);
  _send(bn2[seconddigit + 3], true);

  setCursor(column, 2);
  _send(bn3[firstdigit], true);
  _send(bn3[firstdigit + 1], true);
  _send(bn3[firstdigit + 2], true);
  _send(bn3[firstdigit + 3], true);
  _send(32, true); // Blank
  _send(bn3[seconddigit], true);
  _send(bn3[seconddigit + 1], true);
  _send(bn3[seconddigit + 2], true);
  _send(bn3[seconddigit + 3], true);

  setCursor(column, 3);
  _send(bn4[firstdigit], true);
  _send(bn4[firstdigit + 1], true);
  _send(bn4[firstdigit + 2], true);
  _send(bn4[firstdigit + 3], true);
  _send(32, true); // Blank
  _send(bn4[seconddigit], true);
  _send(bn4[seconddigit + 1], true);
  _send(bn4[seconddigit + 2], true);
  _send(bn4[seconddigit + 3], true);
}

void LiquidCrystal_PCF8574::defineCustomChar()
{
  // 4x4 charset
  uint8_t cc0[8] = {// Custom Character 0
                    B00000,
                    B00000,
                    B00000,
                    B00001,
                    B00011,
                    B00111,
                    B01111,
                    B11111};

  uint8_t cc1[8] = {// Custom Character 1
                    B10000,
                    B11000,
                    B11100,
                    B11110,
                    B11111,
                    B11111,
                    B11111,
                    B11111};

  uint8_t cc2[8] = {// Custom Character 2
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B00000,
                    B00000,
                    B00000};

  uint8_t cc3[8] = {// Custom Character 3
                    B00000,
                    B00000,
                    B00000,
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11111};

  uint8_t cc4[8] = {// Custom Character 4
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B01111,
                    B00111,
                    B00011,
                    B00001};

  uint8_t cc5[8] = {// Custom Character 5
                    B00001,
                    B00011,
                    B00111,
                    B01111,
                    B11111,
                    B11111,
                    B11111,
                    B11111};

  uint8_t cc6[8] = {// Custom Character 6
                    B00000,
                    B00000,
                    B00000,
                    B10000,
                    B11000,
                    B11100,
                    B11110,
                    B11111};

  uint8_t cc7[8] = {// Custom Character 7
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11110,
                    B11100,
                    B11000,
                    B10000};
  createChar(0, cc0);
  createChar(1, cc1);
  createChar(2, cc2);
  createChar(3, cc3);
  createChar(4, cc4);
  createChar(5, cc5);
  createChar(6, cc6);
  createChar(7, cc7);
}

// The End.
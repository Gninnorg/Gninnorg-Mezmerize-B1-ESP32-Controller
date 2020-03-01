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

#include "OLedI2C.h"
#include "Wire.h"
#define OLED_Address 0x3c
#define OLED_Command_Mode 0x80
#define OLED_Data_Mode 0x40

OLedI2C::OLedI2C() {}
OLedI2C::~OLedI2C() {}

void OLedI2C::begin()
{
  PowerUp();
}

void OLedI2C::setCursor(uint8_t col, uint8_t row)
{
  int row_offsets[] = {0x00, 0x20, 0x40, 0x60};
  sendCommand(0x80 | (col + row_offsets[row]));
}

void OLedI2C::clear()
{
  sendCommand(0x01);
}

void OLedI2C::lcdOff()
{
  sendCommand(0x08); // **** Turn Off
}

void OLedI2C::lcdOn()
{
  sendCommand(0x0C); // **** Turn On
}

void OLedI2C::BlinkingCursorOn()
{
  sendCommand(0x0D);
}

void OLedI2C::BlinkingCursorOff()
{
  sendCommand(0x0C); // Same as lcdOn :-)
}

void OLedI2C::FadeOut()
{
  //Set OLED Command set
  sendCommand(0x2A);
  sendCommand(0x79);

  sendCommand(0x23); // Set FadeOut
  sendCommand(0x21);

  sendCommand(0x78); // Exiting Set OLED Command set
  sendCommand(0x28);
}

void OLedI2C::FadeOutCancel()
{
  //Set OLED Command set
  sendCommand(0x2A);
  sendCommand(0x79);

  sendCommand(0x23); // Set FadeOut
  sendCommand(0x00); // Cancel FadeOut

  sendCommand(0x78); // Exiting Set OLED Command set
  sendCommand(0x28);
}

// Write to CGRAM of new characters
void OLedI2C::createChar(uint8_t location, uint8_t charmap[])
{
  location &= 0x7; // we only have 8 locations 0-7

  sendCommand(0x40 | (location << 3));
  delayMicroseconds(30);

  for (int i = 0; i < 8; i++)
  {
    sendData(charmap[i]); // call the virtual write method
    delayMicroseconds(40);
  }
}

void OLedI2C::PowerUp()
{
  // PowerUp function by Rafael Camacho Jan/2014 Brazil

  // Vdd/Vcc off State
  // Power up Vdd

  // Power Stabilized (100ms Delay Minimum)
  delay(100);

  // Initialized State (Parameters as Default)

  // Enable Internal Regulator
  sendCommand(0x2A); // Set "RE" = 1
  sendCommand(0x08);
  sendCommand(0x71); // Function Selection A
  sendData(0x5C);    // 0x00 - Disable        0x5C - Enable       Internal Vdd regulator at 5V I/O application mode

  // Set Display OFF
  sendCommand(0x08);

  // Initial Settings Configuration

  // Set Display Clock Divide Ratio Oscilator Frequency
  sendCommand(0x2A); // Set "RE" = 1
  sendCommand(0x79); // Set "SD" = 1
  sendCommand(0xD5);
  sendCommand(0x70);
  sendCommand(0x78);

  // Set Display Mode
  sendCommand(0x09); // Extended Function Set = Set 5-dot width -> 3 or 4 line(0x09), 1 or 2 line(0x08)

  // Flip display with these two lines, comment out the 0x06 write below
  //sendCommand(0x2A);
  //sendCommand(0x05);   // Set Entry Mode (invert)
  sendCommand(0x06); // Set Entry Mode (normal)

  // CGROM/CGRAM Management
  sendCommand(0x72); // Function Selection B
  sendData(0x01);

  // Set OLED Characterization
  sendCommand(0x79); // Set "SD" = 1

  // Set PEG Pins Hardware Configuration
  sendCommand(0xDA);
  sendCommand(0x10);

  // Set Segment Low Voltage and GPIO
  sendCommand(0xDC); // Function Selection C
  sendCommand(0x03);

  // Set Fade Out and Fade In / Out
  sendCommand(0x23);
  sendCommand(0x00);

  // Vcc Power Stabilized (100ms Delay Recommended)
  delay(100);

  // Set Contrast Control
  sendCommand(0x81);
  sendCommand(0xFF);

  // Set Pre-Charge Period
  sendCommand(0xD9);
  sendCommand(0xF1);

  // Set VCOMH Deselect Level
  sendCommand(0xDB);
  sendCommand(0x30); // 0x00 - ~0.65 x Vcc   0x10 - ~0.71 x Vcc   0x20 - ~0.77 x Vcc   0x30 - ~0.83 x Vcc   0x40 - 1 x Vcc

  // Exiting Set OLED Characterization
  sendCommand(0x78); // Set "SD" = 0
  sendCommand(0x28); // Set "RE" = 0

  // Set Display OFF
  sendCommand(0x08);

  // Clear Display
  sendCommand(0x01);

  // Set DDRAM Address
  sendCommand(0x80);

  // Set Display ON
  sendCommand(0x0C);
}

void OLedI2C::PowerDown()
{
  // Normal Operation

  // Power down Vcc (GPIO)
  sendCommand(0x2A);
  sendCommand(0x79);
  sendCommand(0xDC);
  sendCommand(0x02);

  // Set Display Off
  sendCommand(0x78);
  sendCommand(0x28);
  sendCommand(0x08);

  // (100ms Delay Recommended
  delay(100);

  // Power down Vdd
  // Vdd/Vcc off State
}

void OLedI2C::EnterSleepMode()
{
  // Normal operation

  // Power down Vcc (GPIO)
  sendCommand(0x2A);
  sendCommand(0x79);
  sendCommand(0xDC);
  sendCommand(0x02);

  // Set Display Off
  sendCommand(0x78);
  sendCommand(0x28);
  sendCommand(0x08);

  // Disable Internal Regulator
  sendCommand(0x2A);
  sendCommand(0x71);
  sendData(0x00);
  sendCommand(0x28);

  // Sleep Mode
}

void OLedI2C::ExitingSleepMode()
{
  // Sleep Mode

  // Disable Internal Regulator
  sendCommand(0x2A);
  sendCommand(0x79);
  sendCommand(0x71);
  sendCommand(0x5C);
  sendCommand(0xDC);
  sendCommand(0x03);

  // Power up Vcc (100ms Delay Recommended)
  delay(100);

  // Set Display On
  sendCommand(0x78);
  sendCommand(0x28);
  sendCommand(0x0C);

  // Normal Operation
}

void OLedI2C::sendCommand(uint8_t command)
{
  Wire.beginTransmission(OLED_Address); // **** Start I2C
  Wire.write(OLED_Command_Mode);        // **** Set OLED Command mode
  Wire.write(command);
  Wire.endTransmission(); // **** End I2C
  delay(10);
}

void OLedI2C::backlight(uint8_t contrast) // contrast as 0x00 to 0xFF
{
  //Set OLED Command set
  sendCommand(0x2A);
  sendCommand(0x79);

  sendCommand(0x81);     // Set Contrast
  sendCommand(contrast); // Set contrast value
  sendCommand(0x78);     // Exiting Set OLED Command set
  sendCommand(0x28);
}

/* The write function is needed for derivation from the Print class. */
inline size_t OLedI2C::write(uint8_t ch)
{
  sendData(ch);
  return 1; // assume sucess
} // write()

void OLedI2C::sendData(uint8_t data)
{
  Wire.beginTransmission(OLED_Address); // **** Start I2C
  Wire.write(OLED_Data_Mode);           // **** Set OLED Data mode
  Wire.write(data);
  Wire.endTransmission(); // **** End I2C
}

// Function for printing up tp three 3x3 digits. Works from 000-999 or 00.0-99.9 if decimalPoint is true
void OLedI2C::print3x3Number(uint8_t column, uint8_t row, uint16_t number, uint8_t digits, bool decimalPoint)
{
  uint8_t firstdigit, seconddigit, thirddigit = 0;

  switch (digits)
  {
  case 1:
    firstdigit = number * 3;
    break;
  case 2:
    firstdigit = (number / 10) * 3;
    seconddigit = (number % 10) * 3;
    break;
  default:
    firstdigit = (number / 100) * 3;
    seconddigit = ((number % 100) / 10) * 3;
    thirddigit = ((number % 100) % 10) * 3;
  }

  const uint8_t bn1[] = {5, 2, 6, 32, 5, 32, 2, 2, 6, 2, 2, 6, 31, 32, 31, 31, 2, 2, 5, 2, 2, 2, 2, 6, 5, 2, 6, 5, 2, 6};
  const uint8_t bn2[] = {31, 32, 31, 32, 31, 32, 7, 208, 2, 32, 208, 31, 0, 208, 31, 0, 208, 1, 31, 208, 1, 32, 32, 31, 31, 208, 31, 0, 208, 31};
  const uint8_t bn3[] = {4, 7, 3, 32, 31, 32, 4, 7, 7, 7, 7, 3, 32, 32, 31, 7, 7, 3, 4, 7, 3, 32, 32, 3, 4, 7, 3, 7, 7, 3};

  if (charSet != 1)
    defineCustomChar3x3();

  setCursor(column, row);
  if (number / 100 == 0)
    print("   ");
  else
  {
    sendData(bn1[firstdigit]);
    sendData(bn1[firstdigit + 1]);
    sendData(bn1[firstdigit + 2]);
  }

  if (digits > 1)
  {
    if ((number / 100 == 0) && (number / 10 == 0) && !decimalPoint)
      print("   ");
    else
    {
      sendData(bn1[seconddigit]);
      sendData(bn1[seconddigit + 1]);
      sendData(bn1[seconddigit + 2]);
    }

    if (digits > 2)
    {
      if (decimalPoint)
        sendData(32);
      sendData(bn1[thirddigit]);
      sendData(bn1[thirddigit + 1]);
      sendData(bn1[thirddigit + 2]);
    }
  }

  setCursor(column, row + 1);
  if (number / 100 == 0)
    print("   ");
  else
  {
    sendData(bn2[firstdigit]);
    sendData(bn2[firstdigit + 1]);
    sendData(bn2[firstdigit + 2]);
  }
  if (digits > 1)
  {
    if ((number / 100 == 0) && (number / 10 == 0) && !decimalPoint)
      print("   ");
    else
    {
      sendData(bn2[seconddigit]);
      sendData(bn2[seconddigit + 1]);
      sendData(bn2[seconddigit + 2]);
    }
    if (digits > 2)
    {
      if (decimalPoint)
        sendData(32);
      sendData(bn2[thirddigit]);
      sendData(bn2[thirddigit + 1]);
      sendData(bn2[thirddigit + 2]);
    }
  }

  setCursor(column, row + 2);
  if (number / 100 == 0)
    print("   ");
  else
  {
    sendData(bn3[firstdigit]);
    sendData(bn3[firstdigit + 1]);
    sendData(bn3[firstdigit + 2]);
  }
  if (digits > 1)
  {
    if ((number / 100 == 0) && (number / 10 == 0) && !decimalPoint)
      print("   ");
    else
    {
      sendData(bn3[seconddigit]);
      sendData(bn3[seconddigit + 1]);
      sendData(bn3[seconddigit + 2]);
    }
    if (digits > 2)
    {
      if (decimalPoint)
        sendData(46);
      sendData(bn3[thirddigit]);
      sendData(bn3[thirddigit + 1]);
      sendData(bn3[thirddigit + 2]);
    }
  }
}

void OLedI2C::defineCustomChar3x3()
{
  // 3x3 charset
  uint8_t cc0[8] = {// Custom Character 0
                    B11111,
                    B11111,
                    B11111,
                    B01111,
                    B00111,
                    B00011,
                    B00000,
                    B00000};

  uint8_t cc1[8] = {// Custom Character 1
                    B00000,
                    B10000,
                    B11000,
                    B11100,
                    B11110,
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
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11110,
                    B11100,
                    B11000};

  uint8_t cc4[8] = {// Custom Character 4
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B01111,
                    B00111,
                    B00011};

  uint8_t cc5[8] = {// Custom Character 5
                    B00011,
                    B00111,
                    B01111,
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11111};

  uint8_t cc6[8] = {// Custom Character 6
                    B11000,
                    B11100,
                    B11110,
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11111};

  uint8_t cc7[8] = {// Custom Character 7
                    B00000,
                    B00000,
                    B00000,
                    B11111,
                    B11111,
                    B11111,
                    B11111,
                    B11111};
  createChar(0, cc0);
  createChar(1, cc1);
  createChar(2, cc2);
  createChar(3, cc3);
  createChar(4, cc4);
  createChar(5, cc5);
  createChar(6, cc6);
  createChar(7, cc7);
}

// Function for printing two 4x4 digits. Works from 00-99
void OLedI2C::print4x4Number(uint8_t column, uint8_t number)
{
  uint8_t firstdigit = (number / 10) * 4;
  uint8_t seconddigit = (number % 10) * 4;

  const uint8_t bn1[] = {5, 2, 2, 1, 32, 5, 31, 32, 5, 2, 2, 1, 2, 2, 2, 1, 31, 32, 32, 31, 31, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 31, 5, 2, 2, 1, 5, 2, 2, 1};
  const uint8_t bn2[] = {31, 32, 32, 31, 32, 32, 31, 32, 0, 3, 3, 7, 32, 3, 3, 31, 4, 3, 3, 31, 4, 3, 3, 6, 31, 3, 3, 6, 32, 32, 0, 7, 31, 3, 3, 31, 4, 3, 3, 31};
  const uint8_t bn3[] = {31, 32, 32, 31, 32, 32, 31, 32, 31, 32, 32, 32, 32, 32, 32, 31, 32, 32, 32, 31, 32, 32, 32, 31, 31, 32, 32, 31, 32, 32, 31, 32, 31, 32, 32, 31, 32, 32, 32, 31};
  const uint8_t bn4[] = {4, 3, 3, 7, 32, 3, 31, 3, 4, 3, 3, 3, 4, 3, 3, 7, 32, 32, 32, 31, 4, 3, 3, 7, 4, 3, 3, 7, 32, 32, 31, 32, 4, 3, 3, 7, 4, 3, 3, 7};

  if (charSet != 2)
    defineCustomChar4x4();

  setCursor(column, 0);
  sendData(bn1[firstdigit]);
  sendData(bn1[firstdigit + 1]);
  sendData(bn1[firstdigit + 2]);
  sendData(bn1[firstdigit + 3]);
  sendData(32); // Blank
  sendData(bn1[seconddigit]);
  sendData(bn1[seconddigit + 1]);
  sendData(bn1[seconddigit + 2]);
  sendData(bn1[seconddigit + 3]);

  setCursor(column, 1);
  sendData(bn2[firstdigit]);
  sendData(bn2[firstdigit + 1]);
  sendData(bn2[firstdigit + 2]);
  sendData(bn2[firstdigit + 3]);
  sendData(32); // Blank
  sendData(bn2[seconddigit]);
  sendData(bn2[seconddigit + 1]);
  sendData(bn2[seconddigit + 2]);
  sendData(bn2[seconddigit + 3]);

  setCursor(column, 2);
  sendData(bn3[firstdigit]);
  sendData(bn3[firstdigit + 1]);
  sendData(bn3[firstdigit + 2]);
  sendData(bn3[firstdigit + 3]);
  sendData(32); // Blank
  sendData(bn3[seconddigit]);
  sendData(bn3[seconddigit + 1]);
  sendData(bn3[seconddigit + 2]);
  sendData(bn3[seconddigit + 3]);

  setCursor(column, 3);
  sendData(bn4[firstdigit]);
  sendData(bn4[firstdigit + 1]);
  sendData(bn4[firstdigit + 2]);
  sendData(bn4[firstdigit + 3]);
  sendData(32); // Blank
  sendData(bn4[seconddigit]);
  sendData(bn4[seconddigit + 1]);
  sendData(bn4[seconddigit + 2]);
  sendData(bn4[seconddigit + 3]);
}

void OLedI2C::defineCustomChar4x4()
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

/*
Library to encapsulate use of the OLED 2004 from WIDE.HK
The display is based on the SSD1311.
The display has 20 characters and 4 lines.

The library is based on the sample code downloaded fra WIDE.HK
and the work of Phil Grant, 2013, www.gadjet.co.uk
Scrolling coded contributed by Nathan Chantrell http://nathan.chantrell.net/

Adapted and expanded by Jan Tofft, jan@tofft.dk, 2017
*/

#include "OLedI2C.h" 
#include "Wire.h"
#define OLED_Address 0x3c
#define OLED_Command_Mode 0x80
#define OLED_Data_Mode 0x40

OLedI2C::OLedI2C(){}
OLedI2C::~OLedI2C(){}


void OLedI2C::begin() 
{
  PowerUp();
}
 
 
 void OLedI2C::setCursor(uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x20, 0x40, 0x60 };
  sendCommand(0x80 | (col + row_offsets[row]));
}
 
void OLedI2C::clear()
{
  sendCommand(0x01);
}

void OLedI2C::lcdOff()
{
  sendCommand(0x08);  	// **** Turn Off
}

void OLedI2C::lcdOn()
{
  sendCommand(0x0C);  	// **** Turn On
}

void OLedI2C::FadeOut()
{
  //Set OLED Command set
  sendCommand(0x2A); 
  sendCommand(0x79); 
	
  sendCommand(0x23);  	 // Set FadeOut
  sendCommand(0x21);
  
  sendCommand(0x78);  	 // Exiting Set OLED Command set
  sendCommand(0x28);
}

void OLedI2C::FadeOutCancel()
{
	//Set OLED Command set
  sendCommand(0x2A); 
  sendCommand(0x79); 

  sendCommand(0x23);  	 // Set FadeOut
  sendCommand(0x00);  	 // Cancel FadeOut
  
  sendCommand(0x78);  	 // Exiting Set OLED Command set
  sendCommand(0x28);
}

// Write to CGRAM of new characters
void OLedI2C::createChar(uint8_t location, uint8_t charmap[]) 
{
   location &= 0x7;            // we only have 8 locations 0-7
   
   sendCommand(0x40 | (location << 3));
   delayMicroseconds(30);
   
   for (int i=0; i<8; i++) 
   {
      sendData(charmap[i]);      // call the virtual write method
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
  sendCommand(0x2A);            // Set "RE" = 1
  sendCommand(0x08);
  sendCommand(0x71);            // Function Selection A
  sendData(0x5C);               // 0x00 - Disable        0x5C - Enable       Internal Vdd regulator at 5V I/O application mode             
  
  // Set Display OFF
  sendCommand(0x08);
  
  // Initial Settings Configuration
  
  // Set Display Clock Divide Ratio Oscilator Frequency
  sendCommand(0x2A);            // Set "RE" = 1
  sendCommand(0x79);            // Set "SD" = 1
  sendCommand(0xD5);
  sendCommand(0x70);
  sendCommand(0x78);
  
  // Set Display Mode
  sendCommand(0x09);            // Extended Function Set = Set 5-dot width -> 3 or 4 line(0x09), 1 or 2 line(0x08)
  
  // Set Re-Map
  sendCommand(0x06);            // Set Com31-->Com0  Seg0-->Seg99
  
  // CGROM/CGRAM Management
  sendCommand(0x72);            // Function Selection B
  sendData(0x01);
  
  // Set OLED Characterization
  sendCommand(0x79);            // Set "SD" = 1
  
  // Set PEG Pins Hardware Configuration
  sendCommand(0xDA);
  sendCommand(0x10);
  
  // Set Segment Low Voltage and GPIO
  sendCommand(0xDC);            // Function Selection C
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
  sendCommand(0x30);            // 0x00 - ~0.65 x Vcc   0x10 - ~0.71 x Vcc   0x20 - ~0.77 x Vcc   0x30 - ~0.83 x Vcc   0x40 - 1 x Vcc
  
  // Exiting Set OLED Characterization
  sendCommand(0x78);            // Set "SD" = 0
  sendCommand(0x28);            // Set "RE" = 0

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

void OLedI2C::sendCommand(unsigned char command)
{
  Wire.beginTransmission(OLED_Address); 	 // **** Start I2C 
  Wire.write(OLED_Command_Mode);     		 // **** Set OLED Command mode
  Wire.write(command);
  Wire.endTransmission();                	 // **** End I2C 
  delay(10);
}

void OLedI2C::sendFloat(float digit, uint8_t dec, uint8_t nad, uint8_t col, uint8_t row)
{
  char line[10];//Ten characters, I hope that's enough
  dtostrf(digit,dec,nad,line);//Convert the float value to a string
  sendString(line, col, row);
}

void OLedI2C::backlight(unsigned char contrast) // contrast as 0x00 to 0xFF
{
  //Set OLED Command set
  sendCommand(0x2A); 
  sendCommand(0x79); 
	
  sendCommand(0x81);  	 // Set Contrast
  sendCommand(contrast); // Set contrast value
  sendCommand(0x78);  	 // Exiting Set OLED Command set
  sendCommand(0x28);
}

void OLedI2C::sendString(const char *String, uint8_t col, uint8_t row)
{
  setCursor(col, row);
  unsigned char i=0;
  while(String[i])
  {
    sendData(String[i]);      // *** Show String to OLED
  	i++;
  }
}

void OLedI2C::print(const char *String)
{
  unsigned char i=0;
  while(String[i])
  {
    sendData(String[i]);      // *** Show String to OLED
	i++;
  }
}

void OLedI2C::print(byte value)
{
  char String[4];//Ten characters, I hope that's enough
  unsigned char i=0;
  dtostrf((int)value,3,0,String); 
  while(String[i])
  {
    sendData(String[i]);      // *** Show String to OLED
    i++;
  }
}

void OLedI2C::sendData(unsigned char data)
{
  Wire.beginTransmission(OLED_Address);  	// **** Start I2C 
  Wire.write(OLED_Data_Mode);     		// **** Set OLED Data mode
  Wire.write(data);
  Wire.endTransmission();                     // **** End I2C 
}

void OLedI2C::write(unsigned char data)
{
  sendData(data);
}

void OLedI2C::scrollString(char* message, byte row, unsigned int time)//written by Nathan Chantrell http://nathan.chantrell.net/
{ 
  char buffer[20];
  for (byte i=0;i<strlen(message)+20;i++) {
    byte pos = i+1;
    for (byte j=0;j<20;j++) {
      if ((pos<20)||(pos>strlen(message)+19)) { // pad and trail with blank spaces
        buffer[j]=' ';
      }
      else buffer[j]=message[pos-20];
      pos++;
    }
    sendString(buffer, 0, row);
    delay(time);
  }
}

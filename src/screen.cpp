


/////////////////////////
/******* SCREEN  *******/
/*
  byte sinMap(unsigned long x, int xmin, int xmax, int offset) {
	float pw = (sin(map(x, xmin, xmax, 4712, 6283) / 1000.0) + 1) * (LCDBRI_MAX - offset) + offset;
	return byte(pw);
}

void startLCDFadeIn() {
	if (LCDBacklightMode == LCDSTATE_MIN || LCDBacklightMode == LCDSTATE_FADEOUT) {
		LCDBacklightMode = LCDSTATE_FADEIN;
		lcd.lcdOn();
		mil_onFadeIn = millis();
		LCDinitialPW = LCDcurrentPW;
	}
}

void startLCDFadeOut() {
	if (LCDBacklightMode == LCDSTATE_MAX || LCDBacklightMode == LCDSTATE_FADEIN) {
		LCDBacklightMode = LCDSTATE_FADEOUT;
    lcd.lcdOff();
		mil_onFadeOut = millis();
		LCDinitialPW = LCDcurrentPW;
	}
}

void setLCDMinLight() {
	LCDBacklightMode = LCDSTATE_MIN;
  lcd.lcdOff();
//	analogWrite(PIN_LCDBRI, LCDBRI_MIN);
  LCDcurrentPW = LCDBRI_MIN;
}

void setLCDMaxLight() {
	LCDBacklightMode = LCDSTATE_MAX;
  lcd.lcdOn();
//	analogWrite(PIN_LCDBRI, LCDBRI_MAX);
  LCDcurrentPW = LCDBRI_MAX;
}
*/

void setLCDOn() {
  LCDState = ON;
  lcd.lcdOn();
}

void setLCDOff() {
  LCDState = OFF;
  lcd.lcdOff();
}

// prints a text on the LCD, padding with " " up to size
void lcd_print(char* text, byte size) {
	bool eot = false;
	for (byte i = 0; i < size; i++) {
		if (text[i] == 0)
			eot = true;
		if (eot)
			lcd.write(32);
		else
			lcd.write(text[i]);
	}
}

// prints progress bar
void printBar(byte p){
	if (p == 0) return;
	if (p > 100) p = 100;
	lcd.setCursor(int(((LCDCOLUMNS * 1.0) - 1.0)*p / 100.0), 1);
	lcd.write(31);
}

// 4x4 charset
const byte cc0[8] = {    // Custom Character 0
  B00000,
  B00000,
  B00000,
  B00001,
  B00011,
  B00111,
  B01111,
  B11111
};

const byte cc1[8] = {    // Custom Character 1
  B10000,
  B11000,
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111
};

const byte cc2[8] = {    // Custom Character 2
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000
};

const byte cc3[8] = {    // Custom Character 3
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

const byte cc4[8] = {    // Custom Character 4
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00111,
  B00011,
  B00001
};

const byte cc5[8] = {    // Custom Character 5
  B00001,
  B00011,
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111
};

const byte cc6[8] = {    // Custom Character 6
  B00000,
  B00000,
  B00000,
  B10000,
  B11000,
  B11100,
  B11110,
  B11111
};

const byte cc7[8] = {    // Custom Character 7
  B11111,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100,
  B11000,
  B10000
};

//                    0                1                2                3                4                5                6                7                8                9
const char bn1[] = {  5,  2,  2,  1,  32,  5, 31, 32,   5,  2,  2,  1,   2,  2,  2,  1,  31, 32, 32, 31,  31,  2,  2,  2,   5,  2,  2,  2,   2,  2,  2, 31,   5,  2,  2,  1,   5,  2,  2,  1 };
const char bn2[] = { 31, 32, 32, 31,  32, 32, 31, 32,   0,  3,  3,  7,  32,  3,  3, 31,   4,  3,  3, 31,   4,  3,  3,  6,  31,  3,  3,  6,  32, 32,  0,  7,  31,  3,  3, 31,   4,  3,  3, 31 };
const char bn3[] = { 31, 32, 32, 31,  32, 32, 31, 32,  31, 32, 32, 32,  32, 32, 32, 31,  32, 32, 32, 31,  32, 32, 32, 31,  31, 32, 32, 31,  32, 32, 31, 32,  31, 32, 32, 31,  32, 32, 32, 31 };
const char bn4[] = {  4,  3,  3,  7,  32,  3, 31,  3,   4,  3,  3,  3,   4,  3,  3,  7,  32, 32, 32, 31,   4,  3,  3,  7,   4,  3,  3,  7,  32, 32, 31, 32,   4,  3,  3,  7,   4,  3,  3,  7 };

// Functions for printing two 4x4 digits. Works from 00-99
void printTwoNumber(byte column, byte number) {
  const byte firstdigit = (number / 10) * 4;
  const byte seconddigit = (number % 10) * 4;
  
  lcd.setCursor(column, 0); 
  lcd.sendData(bn1[firstdigit]);
  lcd.sendData(bn1[firstdigit + 1]);
  lcd.sendData(bn1[firstdigit + 2]);
  lcd.sendData(bn1[firstdigit + 3]);
  lcd.sendData(32); // Blank
  lcd.sendData(bn1[seconddigit]);
  lcd.sendData(bn1[seconddigit + 1]);
  lcd.sendData(bn1[seconddigit + 2]);
  lcd.sendData(bn1[seconddigit + 3]);

  lcd.setCursor(column, 1); 
  lcd.sendData(bn2[firstdigit]);
  lcd.sendData(bn2[firstdigit + 1]);
  lcd.sendData(bn2[firstdigit + 2]);
  lcd.sendData(bn2[firstdigit + 3]);
  lcd.sendData(32); // Blank
  lcd.sendData(bn2[seconddigit]);
  lcd.sendData(bn2[seconddigit + 1]);
  lcd.sendData(bn2[seconddigit + 2]);
  lcd.sendData(bn2[seconddigit + 3]);

  lcd.setCursor(column, 2); 
  lcd.sendData(bn3[firstdigit]);
  lcd.sendData(bn3[firstdigit + 1]);
  lcd.sendData(bn3[firstdigit + 2]);
  lcd.sendData(bn3[firstdigit + 3]);
  lcd.sendData(32); // Blank
  lcd.sendData(bn3[seconddigit]);
  lcd.sendData(bn3[seconddigit + 1]);
  lcd.sendData(bn3[seconddigit + 2]);
  lcd.sendData(bn3[seconddigit + 3]);

  lcd.setCursor(column, 3); 
  lcd.sendData(bn4[firstdigit]);
  lcd.sendData(bn4[firstdigit + 1]);
  lcd.sendData(bn4[firstdigit + 2]);
  lcd.sendData(bn4[firstdigit + 3]);
  lcd.sendData(32); // Blank
  lcd.sendData(bn4[seconddigit]);
  lcd.sendData(bn4[seconddigit + 1]);
  lcd.sendData(bn4[seconddigit + 2]);
  lcd.sendData(bn4[seconddigit + 3]);
}

void defineCustomChar() {
    lcd.createChar(0, cc0);  
    lcd.createChar(1, cc1);  
    lcd.createChar(2, cc2);  
    lcd.createChar(3, cc3);  
    lcd.createChar(4, cc4);  
    lcd.createChar(5, cc5);  
    lcd.createChar(6, cc6);  
    lcd.createChar(7, cc7);  
}

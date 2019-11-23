


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



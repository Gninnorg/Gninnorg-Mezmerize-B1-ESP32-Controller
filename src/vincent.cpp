/**********************************************************************************
**
**    VxD optical volume / preamp controller firmware
**
**    v1.0 07.2015 
**
** https://github.com/neu-rah/ArduinoMenu/wiki 
**
**    Uses code from: Malpartida (LCD lib), Hifiduino (IR remote), Adafruit (port expander), GreyGnome (PinChange)
**
**    Free for DIY. No commercial use.
***********************************************************************************/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PinChangeInt.h>
#include <EEPROM.h>
#include "Adafruit_MCP23008.h"
#include "LDRstruct.h"


#pragma region defines

// Please only customize the values where the line comment starts with "//**" (unless you have a good reason)

// Debug mode increases memory usage, the code may not compile. In this case, reduce attenuation steps number to 30.
// !! COMMENT OUT THE FOLLOWING LINE WHEN NOT DEBUGGING ("//#define DEBUG") !!
#define DEBUG


// Set I/O configuration here.
// There can be 0..2 inputs and 0..4 outputs, OR 0..4 inputs and 0..2 outputs.
// So for example we can have 2 IN and 4 OUT, or 3 IN and 2 OUT, or 4 IN and 1 OUT, but NOT 3 IN and 3 OUT.
#define INPUTCOUNT 1 	 //** number of inputs, 0 to 4. If 0, comment out the next line.
char* inputName[INPUTCOUNT] = { "LINE IN" }; //** each name maximum 9 characters. There must be exactly INPUTCOUNT names in the list.

#define OUTPUTCOUNT 1   //** number of outputs, 0 to 4. If 0, comment out the next line.
char* outputName[OUTPUTCOUNT] = { "LINE OUT" }; //** each name maximum 9 characters. There must be exactly OUTPUTCOUNT names in the list.


// If you need a delayed relay contact (for tube filament preheating, for example), uncomment the following #define line.
//#define DELAY 30  //** The value represents the delay in seconds. Maximum 100. Connects PIN_EXT_R6 to GND after DELAY seconds.


/******* Text messages *******/
char msgWelcome1[] = "";              //** <-- maximum 20 characters (line 1)
char msgWelcome2[] = "VxD LDR";       //** <-- maximum 20 characters (line 2)
char msgWelcome3[] = "";              //** <-- maximum 20 characters (line 3)
char msgWelcome4[] = "preamplifier";  //** <-- maximum 20 characters (line 4)

char msgCalib[] = "Calibrating";      //** <-- maximum 17 characters
char msgTest[] = "Self-testing";      //** <-- maximum 17 characters
char msgBias[] = "Adjust BIAS";       //** <-- maximum 19 characters
char msgMeas[] = "Measure LDRs";      //** <-- maximum 19 characters
char msgCalibM[] = "Calibrate";       //** <-- maximum 19 characters
char msgExit[] = "Exit";              //** <-- maximum 19 characters
char msgErr[] = "ERROR ";             //** <-- maximum 17 characters. Last char must be blank
char msgAbort[] = "Aborted.";         //** <-- maximum 19 characters
char msgNoCalib[] = "Please calibrate"; //** <-- maximum 19 characters

// Error codes:
// 1  : left series could not be calibrated
// 2  : right series could not be calibrated
// 3  : left shunt could not be calibrated
// 4  : right shunt could not be calibrated
// 10 : left series LDR too low max value. Adjust trimmer RT1
// 11 : right series LDR too low max value. Adjust trimmer RT3
// 12 : left shunt LDR too low max value. Adjust trimmer RT2
// 13 : right shunt LDR too low max value. Adjust trimmer RT4
// 20 : cannot set LDRs or calibration relays. Analog and/or relay power supplies are not connected?
// 30 : configuration error: INPUTCOUNT and OUTPUTCOUNT out of range


/******* Attenuator control *******/
#define NOM_IMPEDANCE 30000 //** target nominal attenuator impedance, between 5000 and 50000 Ohm. Recommended: 10000. Higher = less precision
#define LOAD_IMPEDANCE 220000 //** input impedance of the amplifier that follows the attenuator. Should not be less than 100K. Default: 220000 ohm
#define VOL_MAX_STEP 25     //** maximum volume steps; range: 20...80. Higher = more memory usage
#define VOL_DEFAULT 5      //** default volume step. Must be <= than MAX


/******* LDR measured values *******/
#define LDR_VOLTAGE 5.0     //** precisely measured value of the +5V supply (with decimal point. Default: 5.0)
#define LDR_R1 10000        //** precisely measured value of R1 resistor (default: 10000 ohm)
#define LDR_R18 10000       //** precisely measured value of R18 resistor (default: 10000 ohm)

// if any measured value is > 200 ohm, replace the LDR. Normal values are around 100 ohm.
#define LDR_LSE_MIN  90     //** measured value of left series LDR R at maximum current
#define LDR_LSH_MIN 129     //** measured value of left shunt LDR R at maximum current
#define LDR_RSE_MIN  93     //** measured value of right series LDR R at maximum current
#define LDR_RSH_MIN 106     //** measured value of right shunt LDR R at maximum current

/******* SCREEN *******/
#define LCDBRI_MAX 90       //** LCD full brightness (1 - 255)
#define LCDBRI_MIN 18       //** LCD eco brightness
#define ROW_IN 0            //** LCD row to display input channel (0 - 3)
#define ROW_OUT 3           //** LCD row to display output channel (0 - 3)


/******* TIMING *******/
#define TIME_LCDFADEIN 1       //** Time in seconds to bring the LCD to full brightness
#define TIME_LCDFADEOUT 10     //** Time in seconds to bring the LCD to dim brightness
#define TIME_LCDFADEAFTER 20   //** Time in seconds after last user input to start fade out
#define TIME_EXITSELECT 5      //** Time in seconds to exit I/O select mode when no activity

//------- do not edit below this point ----------------------------------------------------//

#define TIME_IGNOREREMOTE_VOL  100 // Time in millisec after IR VOLUME before repeating 
#define TIME_IGNOREREMOTE_CMD  400 // Time in millisec after IR command before repeating
#define TIME_SWITCHBOUNCE 250  // Time in millisec to debounce switch
#define TIME_IRBOUNCE 100      // Time in millisec to debounce IR button
#define TIME_DACSETTLE 500     // Time in millisec to wait for the DAC to stabilize after setting output
#define TIME_RELAYLATCH 250    // Time in millisec for latching I/O relay driver to stabilize (0 if not using latching relays)

/******* LDR control *******/
#define LDR_LOW_TOLERANCE 2 // percent between theory and measured R value acceptable for low R values
#define LDR_ZERO_MIN 300000 // minimum acceptable R when PWM=0 
#define LDR_FULL_MAX 200    // maximum acceptable R when PWM=255. 



/******* I/O PORTS *******/
#define PIN_ENC1 2        // Encoder 1
#define PIN_ENC2 4        // Encoder 2
#define PIN_BTN 5         // Encoder switch
#define PIN_LCDBRI 6      // LCD brightness control
#define PIN_REMOTE 12     // IR receiver (remote control) pin
#define PIN_DAC_LSE 11    // left channel series DAC pin
#define PIN_DAC_LSH 10    // left channel shunt DAC pin
#define PIN_DAC_RSE 9     // right channel series DAC pin
#define PIN_DAC_RSH 3     // right channel shunt DAC pin
#define PIN_SENSE_LSE A0  // left channel series sense pin
#define PIN_SENSE_LSH A6  // left channel shunt sense pin
#define PIN_SENSE_RSE A1  // right channel series sense pin
#define PIN_SENSE_RSH A7  // right channel shunt sense pin
#define PIN_BIAS_LSH 8    // left channel shunt bias pin
#define PIN_BIAS_RSH 7    // right channel shunt bias pin
#define PIN_LDR_L A2      // left channel LDR measure pin
#define PIN_LDR_R A3      // right channel LDR measure pin
#define PIN_EXT_CALIB 0   // port extender: calibration relays
#define PIN_EXT_R1 1      // port extender: relay 1
#define PIN_EXT_R2 2      // port extender: relay 2
#define PIN_EXT_R3 3      // port extender: relay 3
#define PIN_EXT_R4 6      // port extender: relay 4
#define PIN_EXT_R5 5      // port extender: relay 5
#define PIN_EXT_R6 4      // port extender: relay 6. Used for delay relay
#define PIN_EXT_BIAS 7    // port extender: BIAS


/****** IR remote codes ******/
#define cIR_UP 5
#define cIR_DOWN 6
#define cIR_LEFT 4
#define cIR_RIGHT 3
#define cIR_CENTER 46
#define cIR_MENU 1
#define cIR_PLAY 47


/******* INTERFACE ********/
// Selection order in normal mode
#define SEL_INPUT 0
#define SEL_OUTPUT 1

//Selection order in setup mode
#define SEL_BIAS 0
#define SEL_MEAS 1
#define SEL_CALIB 2
#define SEL_EXIT 3

// LCD states
#define LCDSTATE_MIN 0
#define LCDSTATE_FADEIN 1
#define LCDSTATE_MAX 2
#define LCDSTATE_FADEOUT 3

#define B 255
#define A 32


#if INPUTCOUNT == 0 && OUTPUTCOUNT == 0
#define VOLCOL 6
#else
#define VOLCOL 11
#endif


#ifdef DEBUG
#define PRINT(x) Serial.print(x)     // console logging
#define PRINTLN(x) Serial.println(x)
#else
#define PRINT(x)
#define PRINTLN(x)
#endif



/******* MACHINE STATES *******/
#define STATE_RUN 0        // normal run state (volume + input)
#define STATE_SETUP 2      // in setup menu mode
#define STATE_ERROR 4
#define STATE_STANDBY 5
#define STATE_OFF 6

/** Udeladt **/
#define STATE_IO 1         // when user selects input/output
#define STATE_CALIB 3      // calibration and measuring 


#define ON HIGH
#define OFF LOW

#pragma endregion defines




///////////////////////////////////////////////////////////////////////////////////////
// VARIABLES
#pragma region variables
/******* GLOBAL *******/
Adafruit_MCP23008 mcp;            // port extender
byte volume;                      // current volume, between 0 and VOL_STEPS
byte chan_in;                     // current input channel. 0 based
byte chan_out;                    // current output channel. 0 based
byte state;                       // current machine state
bool isMuted;			  // current mute status
byte selIO;	     		  // selected interface option in normal mode
byte selSetup;			  // selected interface option in setup mode
byte errc;			  // error code
bool btnReleased;
bool btnPressed;
bool IRPressed;
byte LSHrange;
byte RSHrange;
bool notCalibrated;		  // if no calibration data exists, remain in Setup mode


/******** LDR ********/
//LDR necessary current for each volume step.
// VOL_MAX_STEP + 1 total volume steps, including mute and full
// Step vol=0: MUTE, no data necessary
// Step vol=1: min volume (max series R + min shunt R), no data necessary
// Step vol=VOL_MAX_STEP: full volume (min series R + max shunt R), no data necessary
// So we have VOL_MAX_STEP+1 volume steps, but VOL_MAX_STEP-2 stored samples
// vol=2 -> data[0]
// vol=3 -> data[1]
LDRsample dataL[VOL_MAX_STEP - 2]; // left channel
LDRsample dataR[VOL_MAX_STEP - 2]; // right channel
int deltaSample;
char deltaPW;


/******** LCD ********/
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
byte LCDBacklightMode;            // LCD state 0= low; 1= fade in; 2= high; 3= fade out
byte LCDcurrentPW;                // current backlight pulse width
byte LCDinitialPW;                // pulse width when fade operation started
byte tick;
byte percent;


/******* REMOTE *******/
unsigned long mil_onRemote = 0; //debounce remote

int duration;         // Duration of the IR pulse
int mask;             // Used to decode the remote signals
int c1;               // Byte 1 of the 32-bit remote command code
int c2;               // Byte 2 of the 32-bit remote command code
int c3;               // Byte 3 of the 32-bit remote command code
int IRkey;            // The unique code (Byte 3) of the remote key
int previousIRkey;    // The previous code (used for repeat)
bool isIRrepeat;      // Repeating code


/******* ENCODER *******/
volatile int encoderPos = 0;       // encoder clicks
boolean encoderRotating = false;   // debounce management
volatile boolean A_set = false;
volatile boolean B_set = false;


/******* TIMING *******/
unsigned long mil_onButton;    // Stores last time for switch debounce
unsigned long mil_onAction;    // Stores last time of user input
unsigned long mil_onFadeIn;    // LCD fade timing
unsigned long mil_onFadeOut;   // LCD fade timing
unsigned long mil_onSetLDR;    // Stores last time that a LDR has changed value
unsigned long mil_onInput;     // Last time relay set
unsigned long mil_onOutput;    // Last time relay set
unsigned long mil_delta;

#pragma endregion






///////////////////////////////////////////////////////////////////////////////////////
//  calibration routines

/******* Adjust *******/
#pragma region Adjust
void setMaxResistanceMode() {
	// Sets DACs to 0. Adjust trimmers to measure 500K (when hot)...1M (when cold) on each LDR
	state = STATE_CALIB;
	setCalibrationRelays(OFF);
	//PRINTLN("Adjust bias trimmers");
	setLSE(0); setLSH(0); setRSE(0); setRSH(0);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Adjust bias trimmers");
	lcd.setCursor(0, 1);
	lcd.print(" to 700K each LDR");
	lcd.setCursor(2, 2);
	lcd.print("and re-calibrate");
	lcd.setCursor(2, 3);
	lcd.print("Press btn to end");
}


void setMinResistanceMode() {
	// Sets DACs to maximum. Measure the value of the LDR resistors and manually update the #define LDR_XXX_MIN above.
	state = STATE_CALIB;
	setCalibrationRelays(OFF);
	//PRINTLN("Measure min LDR values");
	setLSE_Range(LOW); setLSH_Range(LOW); setRSE_Range(LOW); setRSH_Range(LOW);
	setLSE(255); setLSH(255); setRSE(255); setRSH(255);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Measure min LDR vals");
	lcd.setCursor(0, 1);
	lcd.print("then update the code");
	lcd.setCursor(2, 2);
	lcd.print("and re-calibrate");
	lcd.setCursor(2, 3);
	lcd.print("Press btn to end");
}
#pragma endregion


/******* Self-test *******/
#pragma region Self-test
byte doSelfTest() {
	//switch calibration relays on to measure LDRs
	setCalibrationRelays(ON);
	//PRINTLN("Self-testing");
	lcd.clear();
	lcd.setCursor((18 - sizeof(msgTest)) / 2, 1);
	lcd.print(msgTest);

	// test if min/max resistance values are within limits
	// test series LDRs max val
	setLSE_Range(HIGH); setRSE_Range(HIGH); setLSE(0); setRSE(0);
	setLSH_Range(LOW); setRSH_Range(LOW); setLSH(255); setRSH(255); //set shunt R to minimum
	delay(3000);
	lcd.print(".");
	unsigned long val;
	val = getRLSE();
	if (val < LDR_ZERO_MIN) {
		//PRINT("Error 10 "); PRINTLN(val);
		return 10;
	}
	val = getRRSE();
	if (val < LDR_ZERO_MIN) {
		//PRINTLN("Error 11 "); PRINTLN(val);
		return 11;
	}


	// test shunt LDRs max val
	setLSE_Range(LOW); setRSE_Range(LOW); setLSE(255); setRSE(255); //set series R to minimum
	setLSH_Range(HIGH); setRSH_Range(HIGH); setLSH(0); setRSH(0);
	delay(4000);
	lcd.print(".");
	val = getRLSH();
	if (val < LDR_ZERO_MIN) {
		//PRINTLN("Error 12 "); PRINTLN(val);
		return 12;
	}
	val = getRRSH();
	if (val < LDR_ZERO_MIN) {
		//PRINTLN("Error 13 "); PRINTLN(val);
		return 13;
	}

	return 0;
}
#pragma endregion


/******* Calibration *******/
#pragma region Calibration
byte doCalibration() {

	//** local vars
	float att;
	unsigned long LSE_target, LSH_target, RSE_target, RSH_target, targetL0, targetR0;
	byte pwL0, pwL1, pwR0, pwR1 = 0;
	word iL0, iL1, iR0, iR1 = 0;
	unsigned long rL0, rL1, rR0, rR1 = 0;
	byte rangeL, rangeR;
	byte dpw, drm;
	bool foundL, foundR = false;
	bool reloopL, reloopR = false;
	bool nojumpL, nojumpR = false;

	setLCDMaxLight();
	state = STATE_CALIB;

	errc = doSelfTest();
	if (errc != 0)
		return errc;

	//PRINTLN("Calibration");
	lcd.clear();
	lcd.setCursor((18 - sizeof(msgCalib)) / 2, 1);
	lcd.print(msgCalib);
	tick = 0;
	percent = 0;
	setCalibrationRelays(ON);

	//** get minimum R in HIGH R range
	setLSE_Range(HIGH); setRSE_Range(HIGH); setLSE(255); setRSE(255); //set series R to High range, minimum value
	setLSH_Range(LOW); setRSH_Range(LOW); setLSH(255); setRSH(255); //set shunt R to minimum
	delay(2000);
	unsigned long LSE_Min_Hi = getRLSE();
	unsigned long RSE_Min_Hi = getRRSE();
	printTick();

	setLSE_Range(LOW); setRSE_Range(LOW); setLSE(255); setRSE(255); //set series R to minimum
	setLSH_Range(HIGH); setRSH_Range(HIGH); setLSH(250); setRSH(250); //set shunt R to High range, almost minimum value
	delay(2000);
	unsigned long LSH_Min_Hi = getRLSH();
	unsigned long RSH_Min_Hi = getRRSH();
	setLSE(0); setLSH(0); setRSE(0); setRSH(0);
	printTick();
	delay(2000);
	printTick();
	//PRINT(LSH_Min_Hi); PRINT("   "); PRINTLN(RSH_Min_Hi);



	//** search for series R current
#pragma region series
	//series: the resistances decrease. start from maximum value and go down.
	rL0 = rR0 = 4000000;
	pwL0 = pwR0 = 0; pwL1 = pwR1 = 1;
	iL0 = iR0 = 0;
	setLSE_Range(HIGH); setRSE_Range(HIGH); setLSE(0); setRSE(0);
	setLSH_Range(LOW); setRSH_Range(LOW); setLSH(255); setRSH(255);


	//loop for each volume sample needed
	for (byte i = 0; i <= VOL_MAX_STEP - 3; i++) {
		//byte step = i+2;


		//** calculate attenuation needed
		att = getAttFromStep(i + 2);

		//** calculate target R values
		LSE_target = RSE_target = getRxFromAttAndImp(att, NOM_IMPEDANCE);
		LSH_target = RSH_target = getRyFromAttAndImp(att, NOM_IMPEDANCE);

		//** adjust shunt R for the parallel load impedance
		LSH_target = LOAD_IMPEDANCE * LSH_target / (LOAD_IMPEDANCE - LSH_target);
		RSH_target = LOAD_IMPEDANCE * RSH_target / (LOAD_IMPEDANCE - RSH_target);

		//** adjust values if out of attainable range
		if (LSE_target < LSE_Min_Hi - LSE_Min_Hi * LDR_LOW_TOLERANCE / 100) {
			LSE_target = LSE_Min_Hi;
			LSH_target = LOAD_IMPEDANCE * getRyFromAttAndRx(att, LSE_target) / (LOAD_IMPEDANCE - getRyFromAttAndRx(att, LSE_target));
		}

		if (LSH_target < LDR_LSH_MIN - LDR_LSH_MIN * LDR_LOW_TOLERANCE / 100) {
			LSH_target = LDR_LSH_MIN;
			LSE_target = getRxFromAttAndRy(att, LSH_target);
		}

		if (RSE_target < RSE_Min_Hi - RSE_Min_Hi * LDR_LOW_TOLERANCE / 100) {
			RSE_target = RSE_Min_Hi;
			RSH_target = LOAD_IMPEDANCE * getRyFromAttAndRx(att, RSE_target) / (LOAD_IMPEDANCE - getRyFromAttAndRx(att, RSE_target));
		}

		if (RSH_target < LDR_RSH_MIN - LDR_RSH_MIN * LDR_LOW_TOLERANCE / 100) {
			RSH_target = LDR_RSH_MIN;
			RSE_target = getRxFromAttAndRy(att, RSH_target);
		}

		PRINT("** Step "); PRINT(i + 2);
		PRINT(" Att= "); PRINT(att);
		PRINT(" LSE_t= "); PRINT(LSE_target); //print the calculated R values
		PRINT(" LSH_t= "); PRINT(LSH_target);
		PRINT(" RSE_t= "); PRINT(RSE_target);
		PRINT(" RSH_t= "); PRINTLN(RSH_target);


		//** find LDR current for the target resistances calculated above
		foundL = foundR = false;

		do {
			reloopL = reloopR = false;  //reloop = if the jump was too large, loop again with step=1

			setLSE(pwL1); setRSE(pwR1);

			if (digitalRead(PIN_BTN) == LOW)
				return 100; // abort on BTN press

			delay(500); // for the DAC to stabilize
			int del = getDelayHi(max(getILSE(), getIRSE())) * 100 - 500; // +variable delay
			//PRINT("del: "); PRINTLN(del);
			if (del > 0) delay(del);
			printTick();

			if (!foundL) {
				rL1 = getRLSE();
				iL1 = getILSE();
			}
			if (!foundR) {
				rR1 = getRRSE();
				iR1 = getIRSE();
			}

			PRINTLN();
			if (!foundL) {
 //       lcd.setCursor(1, 0); //CAG
 //       lcd.print("LSE:"+pwL0+"->"+rL0+"-"+LSE_target); //CAG
				PRINT("LSE: "); PRINT(pwL0);
				PRINT(" -> "); PRINT(rL0);
				PRINT("("); PRINT(iL0);
				PRINT(") - "); PRINT(LSE_target); PRINT(" - ");
				PRINT(pwL1); PRINT(" -> "); PRINT(rL1);
				PRINT("("); PRINT(iL1); PRINTLN(")");
			}
			if (!foundR) {
				PRINT("RSE: "); PRINT(pwR0);
				PRINT(" -> "); PRINT(rR0);
				PRINT("("); PRINT(iR0);
				PRINT(") - "); PRINT(RSE_target); PRINT(" - ");
				PRINT(pwR1); PRINT(" -> "); PRINT(rR1);
				PRINT("("); PRINT(iR1); PRINTLN(")");
			}


			// test if found L
			if (rL0 >= LSE_target && LSE_target >= rL1 && !foundL) {
				if (pwL1 - pwL0 > 1) {
					pwL1 = pwL0 + 1;
					reloopL = true; // found, but the PWM jump was too large, repeat with jump = 1
				}
				else {
					foundL = true;
					if (rL0 - LSE_target >= LSE_target - rL1) {
						dataL[i].pw_SE = pwL1;
						dataL[i].i_SE = iL1;
					}
					else
					{
						dataL[i].pw_SE = pwL0;
						dataL[i].i_SE = iL0;
					}
					PRINT("> Found LSE i="); PRINT(dataL[i].i_SE); PRINTLN();
				}
			}
			if (!foundL && !reloopL)
			{
				if (pwL1 == 255) {
					PRINTLN("Error: LSE not found");
					return 1;
				}

				if (rL0 < LSE_target && i > 1) { //go back
					PRINTLN("Err: OOB LSE");
					pwL0--; pwL1--;
					rL0 = 4000000;
				}
				else
				{
					if (rL0 != rL1 && rL0 != 4000000)
						dpw = abs((rL1 - LSE_target) * (pwL1 - pwL0) / (rL0 - rL1));
					else
						dpw = 1;
					if (dpw == 0) dpw = 1;
					pwL0 = pwL1;
					pwL1 += dpw;


					iL0 = iL1;
					rL0 = rL1;
				}
			}


			// test if found R
			if (rR0 >= RSE_target && RSE_target >= rR1 && !foundR) {
				if (pwR1 - pwR0 > 1) {
					pwR1 = pwR0 + 1;
					reloopR = true;
				}
				else {
					foundR = true;
					if (rR0 - RSE_target >= RSE_target - rR1) {
						dataR[i].pw_SE = pwR1;
						dataR[i].i_SE = iR1;
					}
					else
					{
						dataR[i].pw_SE = pwR0;
						dataR[i].i_SE = iR0;
					}
					PRINT("> Found RSE i="); PRINT(dataR[i].i_SE); PRINTLN();
				}
			}
			if (!foundR && !reloopR)
			{
				if (pwR1 == 255) {
					PRINTLN("Err: RSE not found");
					return 2;
				}

				if (rR0 < RSE_target && i > 1) { //go back
					PRINTLN("Err: OOB RSE");
					pwR0--; pwR1--;
					rR0 = 4000000;
				}
				else
				{
					if (rR0 != rR1 && rR0 != 4000000)
						dpw = (rR1 - RSE_target) * (pwR1 - pwR0) / (rR0 - rR1);
					else
						dpw = 1;
					if (dpw == 0) dpw = 1;
					pwR0 = pwR1;
					pwR1 += dpw;

					iR0 = iR1;
					rR0 = rR1;
				}
			}

		} while (!foundL || !foundR);

		percent = (i + 1) * 100 / (2 * VOL_MAX_STEP - 4);
		printBar(percent);
		//PRINT(percent); PRINTLN("%");

	} //end for series
#pragma endregion series





	//** search for shunt R current
#pragma region shunt
	//shunt: start at min R and go up
	rL0 = rR0 = 0;
	pwL0 = pwR0 = pwL1 = pwR1 = 255;
	iL0 = iR0 = 0;
	targetL0 = targetR0 = 0;
	rangeL = rangeR = LOW;
	setLSE_Range(LOW); setRSE_Range(LOW); setLSE(255); setRSE(255);
	setLSH_Range(LOW); setRSH_Range(LOW); setLSH(255); setRSH(255);
	delay(2000);
	printTick();

	//loop for each volume sample needed
	for (byte i = 0; i <= VOL_MAX_STEP - 3; i++) {
		//byte step = i+2;

		PRINTLN(); PRINT("** Step "); PRINTLN(i + 2);

		//** calculate target R values
		att = getAttFromStep(i + 2);

		LSE_target = RSE_target = getRxFromAttAndImp(att, NOM_IMPEDANCE);
		LSH_target = RSH_target = getRyFromAttAndImp(att, NOM_IMPEDANCE);

		//** adjust shunt R for the parallel load impedance
		LSH_target = LOAD_IMPEDANCE * LSH_target / (LOAD_IMPEDANCE - LSH_target);
		RSH_target = LOAD_IMPEDANCE * RSH_target / (LOAD_IMPEDANCE - RSH_target);

		if (LSE_target < LSE_Min_Hi - LSE_Min_Hi * LDR_LOW_TOLERANCE / 100) {
			LSE_target = LSE_Min_Hi;
			LSH_target = LOAD_IMPEDANCE * getRyFromAttAndRx(att, LSE_target) / (LOAD_IMPEDANCE - getRyFromAttAndRx(att, LSE_target));
		}

		if (LSH_target < LDR_LSH_MIN - LDR_LSH_MIN * LDR_LOW_TOLERANCE / 100) {
			LSH_target = LDR_LSH_MIN;
			LSE_target = getRxFromAttAndRy(att, LSH_target);
		}

		if (RSE_target < RSE_Min_Hi - RSE_Min_Hi * LDR_LOW_TOLERANCE / 100) {
			RSE_target = RSE_Min_Hi;
			RSH_target = LOAD_IMPEDANCE * getRyFromAttAndRx(att, RSE_target) / (LOAD_IMPEDANCE - getRyFromAttAndRx(att, RSE_target));
		}

		if (RSH_target < LDR_RSH_MIN - LDR_RSH_MIN * LDR_LOW_TOLERANCE / 100) {
			RSH_target = LDR_RSH_MIN;
			RSE_target = getRxFromAttAndRy(att, RSH_target);
		}


		//** find LDR current for the target resistances
		foundL = foundR = false;
		nojumpL = nojumpR = false;

		//switch range
		if (rangeL == LOW && (LSH_target > LSH_Min_Hi)) {
			PRINTLN("---> LSH HI range");
			rangeL = HIGH;
			setLSH_Range(HIGH);
			setLSH(255);
			delay(1000);
			rL0 = 0; iL0 = 0;
			pwL0 = pwL1 = 255;
			targetL0 = 0;
		}
		if (rangeR == LOW && (RSH_target > RSH_Min_Hi)) {
			PRINTLN("---> RSH HI range");
			rangeR = HIGH;
			setRSH_Range(HIGH);
			setRSH(255);
			delay(1000);
			rR0 = 0; iR0 = 0;
			pwR0 = pwR1 = 255;
			targetR0 = 0;
		}

		// already found the value?
		if (targetL0 == LSH_target) {
			dataL[i].pw_SH = dataL[i - 1].pw_SH;
			dataL[i].i_SH = dataL[i - 1].i_SH;
			//PRINT("> Copy LSH i="); PRINT(dataL[i].i_SH); PRINTLN();
			foundL = true;
		}
		if (targetR0 == RSH_target) {
			dataR[i].pw_SH = dataR[i - 1].pw_SH;
			dataR[i].i_SH = dataR[i - 1].i_SH;
			//PRINT("> Copy RSH i="); PRINT(dataL[i].i_SH); PRINTLN();
			foundR = true;
		}

		// search
		do {
			reloopL = reloopR = false;  //reloop = if the jump was too large, loop again with step=1
			setLSH(pwL1); setRSH(pwR1);

			if (digitalRead(PIN_BTN) == LOW)
				return 100; // abort on BTN press

			delay(500);
			if (rangeL == HIGH || rangeR == HIGH) {
				int del = getDelayHi(max(getILSH(), getIRSH())) * 100 - 500;
				//PRINT("del "); PRINTLN(del);
				if (del > 0) delay(del);
			}
			printTick();

			if (!foundL) {
				rL1 = getRLSH();
				iL1 = getILSH();
			}
			if (!foundR) {
				rR1 = getRRSH();
				iR1 = getIRSH();
			}

			PRINTLN();
			if (!foundL) {
				PRINT("LSH: ");
				PRINT(pwL0); PRINT("->"); PRINT(rL0); PRINT("("); PRINT(iL0); PRINT(") - ");
				PRINT(LSH_target); PRINT(" - ");
				PRINT(pwL1); PRINT("->"); PRINT(rL1); PRINT("("); PRINT(iL1); PRINTLN(")");
			}
			if (!foundR) {
				PRINT("RSH: ");
				PRINT(pwR0); PRINT("->"); PRINT(rR0); PRINT("("); PRINT(iR0); PRINT(") - ");
				PRINT(RSH_target); PRINT(" - ");
				PRINT(pwR1); PRINT("->"); PRINT(rR1); PRINT("("); PRINT(iR1); PRINTLN(")");
			}


			// test if found L
			if (rL0 <= LSH_target && LSH_target <= rL1 && !foundL) {
				if (pwL0 - pwL1 > 1) {
					pwL1 = pwL0 - 1;
					nojumpL = reloopL = true;
				}
				else {
					foundL = true;
					if (LSH_target - rL0 < rL1 - LSH_target) {
						dataL[i].pw_SH = pwL0;
						dataL[i].i_SH = iL0;
					}
					else {
						dataL[i].pw_SH = pwL1;
						dataL[i].i_SH = iL1;
					}
					targetL0 = LSH_target;
					PRINT("> Found LSH i="); PRINT(dataL[i].i_SH); PRINTLN();
					if (rangeL == LOW)
						dataL[i].i_SH = dataL[i].i_SH | 32768; //bit15 == 1 means LOW R range
				}
			}

			if (!foundL && !reloopL) {
				if (pwL1 == 0) {
					PRINTLN("Err: LSH not found");
					return 3;
				}

				else if (rL0 > LSH_target && pwL0 < 255) { //go back
					PRINTLN("Err: OOB LSH");
					pwL0++; pwL1++;
					rL0 = 0;
				}

				else {
					if (pwL0 - pwL1 == 1 && rL1 - rL0 > 2 && !nojumpL) {
						if (rL0 != rL1 && rL0 != 0)
							dpw = abs((LSH_target - rL1) / (rL1 - rL0) / 2);
						else
							dpw = 1;
						if (dpw == 0) dpw = 1;
					}
					else dpw = 1;
					pwL0 = pwL1;
					pwL1 -= dpw;

					iL0 = iL1;
					rL0 = rL1;
				}
			}


			// test if found R
			if (rR0 <= RSH_target && RSH_target <= rR1 && !foundR) {
				if (pwR0 - pwR1 > 1) {
					pwR1 = pwR0 - 1;
					nojumpR = reloopR = true;
				}
				else {
					foundR = true;
					if (RSH_target - rR0 < rR1 - RSH_target) {
						dataR[i].pw_SH = pwR0;
						dataR[i].i_SH = iR0;
					}
					else {
						dataR[i].pw_SH = pwR1;
						dataR[i].i_SH = iR1;
					}
					targetR0 = RSH_target;
					PRINT("> Found RSH i="); PRINT(dataR[i].i_SH); PRINTLN();
					if (rangeR == LOW)
						dataR[i].i_SH = dataR[i].i_SH | 32768;
				}
			}

			if (!foundR && !reloopR) {
				if (pwR1 == 0) {
					PRINTLN("Err: RSH not found");
					return 4;
				}

				else if (rR0 > RSH_target && pwR0 < 255) { //go back
					PRINTLN("Err: OOB RSH");
					pwR0++; pwR1++;
					rR0 = 0;
				}

				else
				{
					if (pwR0 - pwR1 == 1 && rR1 - rR0 > 2 && !nojumpR) {
						if (rR0 != rR1 && rR0 != 0)
							dpw = abs((RSH_target - rR1) / (rR1 - rR0) / 2);
						else
							dpw = 1;
						if (dpw == 0) dpw = 1;
					}
					else dpw = 1;
					pwR0 = pwR1;
					pwR1 -= dpw;


					iR0 = iR1;
					rR0 = rR1;
				}
			}

		} while (!foundL || !foundR);
		percent = (i + VOL_MAX_STEP - 1) * 100 / (2 * VOL_MAX_STEP - 4);
		printBar(percent);
		//PRINT(percent); PRINTLN("%");

	} //end for shunt
#pragma endregion shunt

	volume = VOL_DEFAULT;
	saveCalibration();
	toRunState();
	setVolume(volume);
	setCalibrationRelays(OFF);

	return 0;
}
#pragma endregion



//////////////////////////////////////////////////////////////////////////////////////
// LDR management
#pragma region LDR

/******* get attenuation factor from step number, LOG law *******/
float getAttFromStep(byte step) {
	float p = step / float(VOL_MAX_STEP);
	float divider = 100.0;
	if (p < 0.4)
		divider = 100.0 + pow((0.4 - p) * 50, 2.1);
	if (step == 2) divider = 1000.0;
	if (step == 3) divider = 600.0;
	return pow(10, p * 2) / divider;
}


/******* read LDR resistor value *******/
unsigned long getResistance(word measure, word seriesLDRres, word refResistor) {
	if (measure >= 4088)
		return 4000000;
	else {
		float v = LDR_VOLTAGE * measure / 4092.0;
		float result = v * refResistor / (LDR_VOLTAGE - v) - seriesLDRres;
		return long(result) + 25;
	}
}


/******* get oversampled 12-bit reading *******/
word getSample(byte pin) {
	word result = 0;
	for (byte i = 0; i < 16; i++) {
		result += analogRead(pin);
		delay(1);
	}
	return result >> 2;
}


/******* get LDR current *******/
word getILSE() {
	return getSample(PIN_SENSE_LSE);
}
word getILSH() {
	return getSample(PIN_SENSE_LSH);
}
word getIRSE() {
	return getSample(PIN_SENSE_RSE);
}
word getIRSH() {
	return getSample(PIN_SENSE_RSH);
}


/******* get LDR resistance *******/
unsigned long getRLSE() {
	return getResistance(getSample(PIN_LDR_L), LDR_LSH_MIN, LDR_R1);
}
unsigned long getRLSH() {
	return getResistance(getSample(PIN_LDR_L), LDR_LSE_MIN, LDR_R1);
}
unsigned long getRRSE() {
	return getResistance(getSample(PIN_LDR_R), LDR_RSH_MIN, LDR_R18);
}
unsigned long getRRSH() {
	return getResistance(getSample(PIN_LDR_R), LDR_RSE_MIN, LDR_R18);
}


/******* get series R from attenuation and impedance *******/
unsigned long getRxFromAttAndImp(float att, unsigned long imp) {
	return long(imp - att * imp);
}

/******* get shunt R from attenuation and impedance *******/
unsigned long getRyFromAttAndImp(float att, unsigned long imp) {
	return long(att * imp);
}

/******* get series R from attenuation and shunt R *******/
unsigned long getRxFromAttAndRy(float att, unsigned long ry) {
	return long(ry * (1 - att) / att);
}

/******* get shunt R from attenuation and series R *******/
unsigned long getRyFromAttAndRx(float att, unsigned long rx) {
	return long(att * rx / (1 - att));
}


/******* set DAC PWM *******/
inline void setLSE(byte val) {
	analogWrite(PIN_DAC_LSE, val);
}
inline void setLSH(byte val) {
	analogWrite(PIN_DAC_LSH, val);
}
inline void setRSE(byte val) {
	analogWrite(PIN_DAC_RSE, val);
}
inline void setRSH(byte val) {
	analogWrite(PIN_DAC_RSH, val);
}


/******* set high current/low resistance bias range *******/
// LOW = low resistance range
inline void setLSE_Range(byte val) {
	pinMode(PIN_SENSE_LSE, val == LOW ? OUTPUT : INPUT);
}
inline void setRSE_Range(byte val) {
	pinMode(PIN_SENSE_RSE, val == LOW ? OUTPUT : INPUT);
}
inline void setLSH_Range(byte val) {
	pinMode(PIN_BIAS_LSH, val == LOW ? OUTPUT : INPUT);
	LSHrange = val;
}
inline void setRSH_Range(byte val) {
	pinMode(PIN_BIAS_RSH, val == LOW ? OUTPUT : INPUT);
	RSHrange = val;
}


/** average delay needed for LDR settling **/
byte getDelayHi(word i) {
	if (i < 136) return 100;
	if (i < 240) return 70;
	if (i < 290) return 60;
	if (i < 340) return 50;
	if (i < 390) return 40;
	if (i < 590) return 30;
	if (i < 700) return 25;
	if (i < 900) return 20;
	if (i < 1300) return 15;
	if (i < 2100) return 10;
	if (i < 2500) return 8;
	else return 5;
}

word getDelayLo(byte pw) {
	return map(pw, 0, 255, 500, 700);
}

/** average difference in I (current) samples when PWM is increased by 1 **/
byte getDeltaI(word sample) {
	word s = sample & 32767;
	if (sample < 32768) {
		if (s < 42) return 5;
		if (s < 57) return 6;
		if (s < 100) return 7;
		if (s < 150) return 8;
		if (s < 210) return 9;
		if (s < 500) return 10;
		else return 11;
	}
	else {
		if (s < 16) return 3;
		if (s < 32) return 4;
		if (s < 50) return 5;
		if (s < 70) return 6;
		if (s < 100) return 7;
		if (s < 180) return 8;
		if (s < 300) return 9;
		if (s < 640) return 10;
		else return 11;
	}
}

#pragma endregion



//////////////////////////////////////////////////////////////////////////////////////
// RELAYS and I/O control
#pragma region Relays

byte relayMap[5] = { PIN_EXT_R1, PIN_EXT_R2, PIN_EXT_R3, PIN_EXT_R4, PIN_EXT_R5 };

void setCalibrationRelays(byte val) {
	mcp.digitalWrite(PIN_EXT_CALIB, val);
	isMuted = val > 0;
}

void setMute(byte volume) {
	isMuted = volume == 0;
	mcp.digitalWrite(PIN_EXT_CALIB, isMuted);
	printTwoNumber(VOLCOL, volume);
}

void setInput() {
#if INPUTCOUNT > 0
	if (millis() - mil_onInput > TIME_RELAYLATCH) {
		lcd.setCursor(1, ROW_IN);
		lcd_print(inputName[chan_in], 9);

#if INPUTCOUNT > 2 // inputs are on R1 to R4
		for (byte i = 0; i < INPUTCOUNT; i++)
			if (i != chan_in)
				mcp.digitalWrite(relayMap[i], LOW);
		mcp.digitalWrite(relayMap[chan_in], HIGH);

#elif INPUTCOUNT <= 2 && OUTPUTCOUNT <= 2 // inputs are on R1
		if (chan_in)
			mcp.digitalWrite(PIN_EXT_R1, HIGH);
		else
			mcp.digitalWrite(PIN_EXT_R1, LOW);

#else // inputs are on R5
		if (chan_in)
			mcp.digitalWrite(PIN_EXT_R5, HIGH);
		else
			mcp.digitalWrite(PIN_EXT_R5, LOW);
#endif
		mil_onInput = millis();
	}
#endif
}

void setOutput() {
#if	OUTPUTCOUNT > 0
	if (millis() - mil_onOutput > TIME_RELAYLATCH) {
		lcd.setCursor(1, ROW_OUT);
		lcd_print(outputName[chan_out], 9);

#if OUTPUTCOUNT > 2 // outputs are on R1 to R4
		for (byte i = 0; i < OUTPUTCOUNT; i++)
			if (i != chan_out)
				mcp.digitalWrite(relayMap[i], LOW);
		mcp.digitalWrite(relayMap[chan_out], HIGH);

#else // outputs are on R5
		if (chan_out)
			mcp.digitalWrite(PIN_EXT_R5, HIGH);
		else
			mcp.digitalWrite(PIN_EXT_R5, LOW);
#endif
		mil_onOutput = millis();
	}
#endif
}

void setIO() {
	setInput();
	setOutput();
}
#pragma endregion



///////////////////////////////////////////////////////////////////////////////////////
//  Delayed startup
#pragma region delay
#ifdef DELAY
void doDelay() {
	lcd.clear();

// Turns on R6 relay after a DELAY number of seconds
        for (int i = DELAY; i > 0; i--)	{
	  printTwoNumber(6, i);
	  delay(1000);
	}
	mcp.digitalWrite(PIN_EXT_R6, HIGH);

	lcd.clear();
}
#endif
#pragma endregion



//////////////////////////////////////////////////////////////////////////////////////
// IR Remote
#pragma region Remote
/*
IR code by Hifiduino. We cannot use IRremote lib because of timers interference.

The following function returns the code from the Apple Aluminum remote control. The Apple remote is
based on the NEC infrared remote protocol. Of the 32 bits (4 bytes) coded in the protocol, only the
third byte corresponds to the keys. The function also handles errors due to noise (returns 255) and
the repeat code (returs zero)

The Apple remote returns the following codes:

Up key:     238 135 011 089
Down key:   238 135 013 089
Left key:   238 135 008 089
Right key:  238 135 007 089
Center key: 238 135 093 089 followed by 238 135 004 089 (See blog for why there are two codes)
Menu key:   238 135 002 089
Play key:   238 135 094 089 followed by 238 135 004 089

(update) The value of the third byte varies from remote to remote. It turned out that the 8th bit
is not part of the command, so if we only read the 7 most significant bits, the value is the same
for all the remotes, including the white platic remote.

The value for the third byte when we discard the least significant bit is:

Up key:     238 135 005 089
Down key:   238 135 006 089
Left key:   238 135 004 089
Right key:  238 135 003 089
Center key: 238 135 046 089 followed by 238 135 002 089 (See blog for why there are two codes)
Menu key:   238 135 001 089
Play key:   238 135 047 089 followed by 238 135 002 089

More info here: http://hifiduino.wordpress.com/apple-aluminum-remote/
*/



int getIRkey() {

	c1 = c2 = c3 = 0;
	duration = 1;

	while ((duration = pulseIn(PIN_REMOTE, HIGH, 15000)) < 2000 && duration != 0)
	{
		// Wait for start pulse
	}
	if (duration == 0)							// This is an error no start or end of pulse
		return (255);							// Use 255 as Error

	else if (duration < 3000)						// This is the repeat
		return (0);							// Use zero as the repeat code

	else if (duration < 5000) {						// This is the command get the 4 byte
		mask = 1;
		for (int i = 0; i < 8; i++) {					// get 8 bits
			if (pulseIn(PIN_REMOTE, HIGH, 3000) > 1000)  // If "1" pulse
				c1 |= mask;					// Put the "1" in position
			mask <<= 1;						// shift mask to next bit
		}
		mask = 1;
		for (int i = 0; i < 8; i++) {					// get 8 bits
			if (pulseIn(PIN_REMOTE, HIGH, 3000) > 1000)  // If "1" pulse
				c2 |= mask;					// Put the "1" in position
			mask <<= 1;						// shift mask to next bit
		}
		mask = 1;
		for (int i = 0; i < 8; i++) {					// get 8 bits
			if (pulseIn(PIN_REMOTE, HIGH, 3000) > 1000)  // If "1" pulse
				c3 |= mask;					// Put the "1" in position
			mask <<= 1;      					// shift mask to next bit
		}

		c3 >>= 1;                                                       // Discard the least significant bit
		return (c3);
	}
}
#pragma endregion



///////////////////////////////////////////////////////////////////////////////////////
//  EEPROM routines
#pragma region EEPROM

// address:
// 0  volume
// 1  input (0 based)
// 2  output (0 based)
// 3  33 marker
// 4  44 marker
// 5  VOL_MAX_STEP
// 6...  calibration data

#define OFFSET_L 6

void saveCalibration() {

	byte *p;

	//PRINTLN("Save left");
	for (byte sample = 0; sample <= VOL_MAX_STEP - 3; sample++) {
		p = (byte*)&dataL[sample];
		for (byte i = 0; i < 6; i++) {  // LDRsample struct size = 6
			//PRINT("addr "); PRINT(OFFSET_L + sample * 6 + i); PRINT(" byte "); PRINTLN(p[i]);
			if (EEPROM.read(OFFSET_L + sample * 6 + i) != p[i])
				EEPROM.write(OFFSET_L + sample * 6 + i, p[i]);
		}
	}

	//PRINTLN("Save right");
	word offset_R = OFFSET_L + 6 * (VOL_MAX_STEP - 2);

	for (byte sample = 0; sample <= VOL_MAX_STEP - 3; sample++) {
		p = (byte*)&dataR[sample];
		for (byte i = 0; i < 6; i++) {  // LDRsample struct size = 6
			//PRINT("addr "); PRINT(offset_R + sample * 6 + i); PRINT(" byte "); PRINTLN(p[i]);
			if (EEPROM.read(offset_R + sample * 6 + i) != p[i])
				EEPROM.write(offset_R + sample * 6 + i, p[i]);
		}
	}

	//write marker:
	EEPROM.write(3, 33); EEPROM.write(4, 44); EEPROM.write(5, VOL_MAX_STEP);
}


bool loadCalibration() {
	byte *p;
	word offset_R = OFFSET_L + 6 * (VOL_MAX_STEP - 2);

	if (EEPROM.read(3) != 33 || EEPROM.read(4) != 44 || EEPROM.read(5) != VOL_MAX_STEP)
		return false; //no data

	//PRINTLN("Load left");
	for (byte sample = 0; sample <= VOL_MAX_STEP - 3; sample++) {
		p = (byte*)&dataL[sample];
		for (byte i = 0; i < 6; i++) {
			p[i] = EEPROM.read(OFFSET_L + sample * 6 + i);
			//PRINT("addr "); PRINT(OFFSET_L + sample * 6 + i); PRINT(" byte "); PRINTLN(p[i]);
		}
	}

	//PRINTLN("Load right");
	for (byte sample = 0; sample <= VOL_MAX_STEP - 3; sample++) {
		p = (byte*)&dataR[sample];
		for (byte i = 0; i < 6; i++) {
			p[i] = EEPROM.read(offset_R + sample * 6 + i);
			//PRINT("addr "); PRINT(offset_R + sample * 6 + i); PRINT(" byte "); PRINTLN(p[i]);
		}
	}

	return true;
}


void saveIOValues() {
	if (EEPROM.read(0) != volume)
		EEPROM.write(0, volume);
	if (EEPROM.read(1) != chan_in)
		EEPROM.write(1, chan_in);
	if (EEPROM.read(2) != chan_out)
		EEPROM.write(2, chan_out);
}

void loadIOValues() {
	volume = EEPROM.read(0);
	chan_in = EEPROM.read(1);
	chan_out = EEPROM.read(2);
	if (volume > VOL_MAX_STEP)
		volume = VOL_DEFAULT;
	if (chan_in >= INPUTCOUNT)
		chan_in = 0;
	if (chan_out >= OUTPUTCOUNT)
		chan_out = 0;
}


//void eraseEEPROM() {
//	for (byte i = 0; i < 1024; i++) //1024 max
//		EEPROM.write(i, 255);
//}
#pragma endregion



///////////////////////////////////////////////////////////////////////////////////////
//  Encoder routines
#pragma region Encoder
void attachInterrupts() {
	PCintPort::attachInterrupt(PIN_ENC2, &doEncoderB, CHANGE);
	attachInterrupt(0, doEncoderA, CHANGE);
}

void doEncoderA() {
	if (encoderRotating) delay(1);
	if (digitalRead(PIN_ENC2) != A_set) {
		A_set = !A_set;
		if (A_set && !B_set)
			encoderPos += 1;

		encoderRotating = false;
	}
}

void doEncoderB() {
	if (encoderRotating) delay(1);
	if (digitalRead(PIN_ENC1) != B_set) {
		B_set = !B_set;
		if (B_set && !A_set)
			encoderPos -= 1;

		encoderRotating = false;
	}
}
#pragma endregion



///////////////////////////////////////////////////////////////////////////////////////
//  Pin modes
#pragma region pinModes
void setPinModes() {

	Wire.begin();
	mcp.begin();
	mcp.pinMode(PIN_EXT_CALIB, OUTPUT);
	setCalibrationRelays(ON);
	mcp.pinMode(PIN_EXT_R1, OUTPUT);
	mcp.pinMode(PIN_EXT_R2, OUTPUT);
	mcp.pinMode(PIN_EXT_R3, OUTPUT);
	mcp.pinMode(PIN_EXT_R4, OUTPUT);
	mcp.pinMode(PIN_EXT_R5, OUTPUT);
	mcp.pinMode(PIN_EXT_R6, OUTPUT);
	mcp.pinMode(PIN_EXT_BIAS, OUTPUT);

	pinMode(PIN_BIAS_LSH, INPUT);
	pinMode(PIN_BIAS_RSH, INPUT);
	digitalWrite(PIN_BIAS_LSH, LOW);
	digitalWrite(PIN_BIAS_RSH, LOW);
	LSHrange = RSHrange = HIGH;

	pinMode(PIN_SENSE_LSE, INPUT);
	pinMode(PIN_SENSE_LSH, INPUT);
	pinMode(PIN_SENSE_RSE, INPUT);
	pinMode(PIN_SENSE_RSH, INPUT);

	pinMode(PIN_LDR_L, INPUT);
	pinMode(PIN_LDR_R, INPUT);

	pinMode(PIN_ENC1, INPUT_PULLUP);
	pinMode(PIN_ENC2, INPUT_PULLUP);
	pinMode(PIN_BTN, INPUT_PULLUP);

	pinMode(PIN_REMOTE, INPUT);

	pinMode(PIN_DAC_LSE, OUTPUT);
	pinMode(PIN_DAC_LSH, OUTPUT);
	pinMode(PIN_DAC_RSE, OUTPUT);
	pinMode(PIN_DAC_RSH, OUTPUT);
	analogWrite(PIN_DAC_LSE, 0);
	analogWrite(PIN_DAC_LSH, 0);
	analogWrite(PIN_DAC_RSE, 0);
	analogWrite(PIN_DAC_RSH, 0);

	pinMode(PIN_LCDBRI, OUTPUT);
	analogWrite(PIN_LCDBRI, 0);
}
#pragma endregion



///////////////////////////////////////////////////////////////////////////////////////
//  state transitions
#pragma region Transitions

/** transition to setup mode, display menu **/
void toSetupState() {
	selSetup = SEL_BIAS;
	lcd.clear();
	lcd.write(255); lcd.print(msgBias);
	lcd.setCursor(0, 1);
	lcd.write(165); lcd.print(msgMeas);
	lcd.setCursor(0, 2);
	lcd.write(165); lcd.print(msgCalibM);
	lcd.setCursor(0, 3);
	lcd.write(165); lcd.print(msgExit);
	state = STATE_SETUP;
}

/** transition to error state **/
void toErrorState() {
	setCalibrationRelays(ON);
	setLSE(0); setRSE(0);
	setLSH(0); setRSH(0);
	lcd.clear();
	lcd.setCursor(6, 1);
	lcd.print(msgErr); lcd.print(errc);
	state = STATE_ERROR;
}

/** transition to I/O channel adjust mode **/
void toIOState() {
	if (state != STATE_IO) {
		selIO = SEL_OUTPUT;
		if (INPUTCOUNT > 1 || OUTPUTCOUNT > 1) {
			if (INPUTCOUNT > 1) {
				selIO = SEL_INPUT;
				lcd.setCursor(0, ROW_IN); lcd.write(255);
			}
			else if (OUTPUTCOUNT > 1) {
				lcd.setCursor(0, ROW_OUT); lcd.write(255);
			}
		}
		state = STATE_IO;
	}
}

/** transition to normal volume adjust mode **/
void toRunState() {
	if (state != STATE_IO) {
		lcd.clear();
		setVolume(volume);
#if INPUTCOUNT > 0
		lcd.setCursor(1, ROW_IN);
		lcd.print(inputName[chan_in]);
#endif
#if OUTPUTCOUNT > 0
		lcd.setCursor(1, ROW_OUT);
		lcd.print(outputName[chan_out]);
#endif
	}
#if INPUTCOUNT > 0
	lcd.setCursor(0, ROW_IN);
	lcd.write(126);
#endif
#if OUTPUTCOUNT > 0
	lcd.setCursor(0, ROW_OUT);
	lcd.write(127);
#endif


	mil_onAction = millis();
	state = STATE_RUN;
}

#pragma endregion



///////////////////////////////////////////////////////////////////////////////////////
//  volume
#pragma region Volume
/******* set LDRs to current volume *******/
void setVolume(byte vol) {

	bool goHighL, goHighR, goLowL, goLowR;
	byte i = vol - 2;

	if (isMuted == vol || (isMuted && vol))
		setMute(vol);

	if (vol > 0) {
		if (vol == 1) {
			setLSE_Range(HIGH); setRSE_Range(HIGH);
			setLSH_Range(LOW); setRSH_Range(LOW);
			setLSE(2); setRSE(2); setLSH(200); setRSH(200);
		}
		else if (vol == VOL_MAX_STEP) {
			setLSE_Range(LOW); setRSE_Range(LOW);
			setLSH_Range(HIGH); setRSH_Range(HIGH);
			setLSE(255); setRSE(255); setLSH(0); setRSH(0);
		}
		else {
			setLSE_Range(HIGH); setRSE_Range(HIGH);
			setLSE(dataL[i].pw_SE); setRSE(dataR[i].pw_SE);
			setLSH(dataL[i].pw_SH); setRSH(dataR[i].pw_SH);


			goLowL = dataL[i].i_SH >= 32768 && LSHrange == HIGH;
			goLowR = dataR[i].i_SH >= 32768 && RSHrange == HIGH;
			goHighL = dataL[i].i_SH < 32768 && LSHrange == LOW;
			goHighR = dataR[i].i_SH < 32768 && RSHrange == LOW;

			if (goLowL) setLSH_Range(LOW);
			else if (goHighL) setLSH_Range(HIGH);

			if (goLowR) setRSH_Range(LOW);
			else if (goHighR) setRSH_Range(HIGH);

			mil_onSetLDR = millis();
		}

		printTwoNumber(VOLCOL, vol);
	}
}

#pragma endregion





////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//   SETUP
#pragma region setup
void setup() {
	setPinModes();

	setLSE_Range(LOW); setRSE_Range(LOW); setLSE(255); setRSE(255);
	setLSH_Range(LOW); setRSH_Range(LOW); setLSH(255); setRSH(255);

	// set 31250Hz PWM frequency on pins 3,9,10,11
	TCCR1B = TCCR1B & 0b11111000 | 0x01;
	TCCR2B = TCCR2B & 0b11111000 | 0x01;


#ifdef DEBUG
	Serial.begin(57600);
  Serial.println("Setup()");
#endif

	// display welcome message
	lcd.begin(20, 4);
	lcd.clear();
	lcd.backlight();
	lcd.setCursor((21 - sizeof(msgWelcome1)) / 2, 0);
	lcd.print(msgWelcome1);
	lcd.setCursor((21 - sizeof(msgWelcome2)) / 2, 1);
	lcd.print(msgWelcome2);
	lcd.setCursor((21 - sizeof(msgWelcome3)) / 2, 2);
	lcd.print(msgWelcome3);
	lcd.setCursor((21 - sizeof(msgWelcome4)) / 2, 3);
	lcd.print(msgWelcome4);

  PRINTLN("LCD started");
  
	//fade in the LCD
	for (byte i = 0; i <= LCDBRI_MAX; i++) {
		analogWrite(PIN_LCDBRI, sinMap(i, 0, LCDBRI_MAX, 0));
		delay(15);
	}
#if LCDBRI_MAX < 40
	delay(400);
#endif
	defineCustomChar();
	lcd.clear();
	setLCDMaxLight();

	//test if the relays and LDRs are powered
	if (getRLSE() + getRRSE() + getRLSH() + getRRSH() > LDR_FULL_MAX * 4) {
		errc = 20;
		toErrorState();
	}
	else if ((INPUTCOUNT > 4 && OUTPUTCOUNT > 2) || (INPUTCOUNT > 2 && OUTPUTCOUNT > 4) || (INPUTCOUNT == 3 && OUTPUTCOUNT == 3)) {
		errc = 30;
		toErrorState();
	}
	else
	{
		notCalibrated = !loadCalibration();
		if (notCalibrated)
			toSetupState();

		else {
#ifdef DELAY
			setLSE(50); setRSE(50);
			setLSH(50); setRSH(50);
			doDelay();
#endif
			loadIOValues();
			state = STATE_IO;
			setIO();
			isMuted = volume == 0;
			setVolume(volume);
			delay(250);
			setMute(volume);
			toRunState();
		}
	}


	mil_onAction = millis(); // begin counting to dim the LCD

	mcp.digitalWrite(PIN_EXT_BIAS, HIGH); // turn biasing voltage on

	btnReleased = true;
	btnPressed = false;
	IRPressed = false;
	attachInterrupts(); // start reading encoder
	encoderPos = 0;
}
#pragma endregion




////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//         LOOP
#pragma region loop
void loop() {

#pragma region powerOff
	// Detect power off
	if (state == STATE_RUN) {
		long vcc;
		ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
		delay(2);
		ADCSRA |= _BV(ADSC);
		while (bit_is_set(ADCSRA, ADSC));
		vcc = ADCL;
		vcc |= ADCH << 8;
		vcc = 1126400L / vcc;
		if (vcc > 3000 && vcc < 4600) {
			mcp.digitalWrite(PIN_EXT_BIAS, LOW);
			setLSE(0); setRSE(0);
			setLSH_Range(LOW); setRSH_Range(LOW);
			setLSH(255); setRSH(255);
			analogWrite(PIN_LCDBRI, 0);
			saveIOValues();
			state = STATE_OFF;
		}
	}
#pragma endregion


	/////////////////////////////////////////////////////////////////////////////////////
	// IR Remote
#pragma region loop_remote
	if (millis() - mil_onRemote > TIME_IGNOREREMOTE_CMD && digitalRead(PIN_REMOTE) == LOW && (state == STATE_RUN || state == STATE_IO)) {

		IRkey = getIRkey();
		if (IRkey != 255 && IRkey != 2) {

			mil_onAction = millis();
			startLCDFadeIn();


			isIRrepeat = IRkey == 0;
			if (isIRrepeat && (previousIRkey == cIR_UP || previousIRkey == cIR_DOWN))  // Repeat the specified keys
				IRkey = previousIRkey;
			else
				previousIRkey = IRkey;

			//PRINT("IR: "); PRINTLN(IRkey);

			switch (IRkey) {
			case cIR_UP:  // volume up
				encoderPos++;
				break;
			case cIR_DOWN:  // volume down
				encoderPos--;
				break;
			case cIR_LEFT:
				if (INPUTCOUNT > 1 || OUTPUTCOUNT > 1) {
					toIOState();
					encoderPos--;
				}
				break;
			case cIR_RIGHT:
				if (INPUTCOUNT > 1 || OUTPUTCOUNT > 1) {
					toIOState();
					encoderPos++;
				}
				break;
			case cIR_PLAY:
				if (isMuted)
					setVolume(volume);
				else
					setMute(0);
				break;
			case cIR_MENU:
				toRunState();
				break;
			} // end SWITCH

			if (!isIRrepeat) {
				mil_onRemote = millis();
			}
		}

	}
	else
		IRkey = 255;
#pragma endregion



	/////////////////////////////////////////////////////////////////////////////////////
	// Rotary encoder was turned
#pragma region loop_encoder
	encoderRotating = true;
	if (encoderPos != 0)
	{
		mil_onAction = millis();
		startLCDFadeIn();


		/** encoder rotated in volume mode **/
		if (state == STATE_RUN) {
			volume += encoderPos;
			if (volume > 250)	// byte overflow...
				volume = 0;
			if (volume > VOL_MAX_STEP)
				volume = VOL_MAX_STEP;
			setVolume(volume);
		}


		/** encoder rotated in channel select mode **/
		else if (state == STATE_IO) {
			switch (selIO % 2) {
			case SEL_INPUT:
				if (INPUTCOUNT > 0) {
					chan_in += encoderPos;
					if (chan_in == 255)
						chan_in = INPUTCOUNT - 1;
					else
						chan_in %= INPUTCOUNT;
					setInput();
				}
				break;

			case SEL_OUTPUT:
				if (OUTPUTCOUNT > 0) {
					chan_out += encoderPos;
					if (chan_out == 255)
						chan_out = OUTPUTCOUNT - 1;
					else
						chan_out %= OUTPUTCOUNT;
					setOutput();
				}
			}
		}


		/** encoder rotated in "setup" mode **/
		else if (state == STATE_SETUP) {
			lcd.setCursor(0, selSetup); lcd.write(165);
			selSetup += encoderPos;
			selSetup %= 4;
			lcd.setCursor(0, selSetup); lcd.write(255);
		}


		encoderPos = 0; // Reset the flag
	}  // End of encoder rotation
#pragma endregion



	/////////////////////////////////////////////////////////////////////////////////////
	// Button press
#pragma region loop_button
	btnPressed = digitalRead(PIN_BTN) == LOW && btnReleased;
	IRPressed = IRkey == cIR_CENTER && millis() - mil_onButton > TIME_IRBOUNCE;

	if (btnPressed || IRPressed) {

		btnReleased = false;
		mil_onButton = mil_onAction = millis();    // Start debounce timer
		startLCDFadeIn();

		/** button pressed in normal mode **/
		if (state == STATE_RUN)
			if (INPUTCOUNT <= 1 && OUTPUTCOUNT <= 1) {
				if (btnPressed)
					toSetupState();
			}
			else
				toIOState();


		/** button pressed in IO mode **/
		else if (state == STATE_IO) {
			if (selIO == SEL_INPUT && OUTPUTCOUNT > 1) {
				selIO = SEL_OUTPUT;
				lcd.setCursor(0, ROW_IN); lcd.write(126);
				lcd.setCursor(0, ROW_OUT); lcd.write(255);
			}
			else if (btnPressed)
				toSetupState();  //switch to setup mode
			else
				toRunState();
		}

		/** button pressed in setup menu **/
		else if (btnPressed && state == STATE_SETUP) {
			switch (selSetup) {
			case SEL_BIAS:
				setMaxResistanceMode();
				break;
			case SEL_MEAS:
				setMinResistanceMode();
				break;
			case SEL_CALIB:
				errc = doCalibration();
				if (errc == 100) {
					lcd.clear();
					lcd.setCursor(6, 1);
					lcd.print(msgAbort);
					delay(3000);
					toRunState();
					setVolume(volume);
					setCalibrationRelays(OFF);
				}
				else if (errc > 0) {
					toErrorState();
				}
				break;
			case SEL_EXIT:
				if (!notCalibrated)
					toRunState();
				else {
					lcd.clear();
					lcd.setCursor((18 - sizeof(msgNoCalib)) / 2, 1);
					lcd.print(msgNoCalib);
					delay(1000);
					toSetupState();
				}
				break;
			}
		}

		/** button pressed in adjust mode **/
		else if (btnPressed && state == STATE_CALIB)
			toSetupState();
	}
#pragma endregion



	/////////////////////////////////////////////////////////////////////////////////////
	// timing
#pragma region loop_timing
	//** timeout selection/menu
	if ((state == STATE_IO) && millis() - mil_onAction > TIME_EXITSELECT * 1000) {
		saveIOValues();  // after modifying I/O, save to EEPROM
		toRunState();
	}


	//** encoder button debounce/release
	if (digitalRead(PIN_BTN) == HIGH && millis() - mil_onButton > TIME_SWITCHBOUNCE) {
		btnReleased = true;
		//mil_onButton = millis();
	}


	//** Fade in LCD
	if (LCDBacklightMode == LCDSTATE_FADEIN) {
		mil_delta = millis() - mil_onFadeIn;
		if (mil_delta <= TIME_LCDFADEIN * 1000) {
			LCDcurrentPW = sinMap(mil_delta, 0, TIME_LCDFADEIN * 1000, LCDinitialPW);
			analogWrite(PIN_LCDBRI, LCDcurrentPW);
		}
		else
			setLCDMaxLight();
	}


	//** Fade out LCD
	if (LCDBacklightMode == LCDSTATE_FADEOUT) {
		mil_delta = millis() - mil_onFadeOut;
		if (mil_delta <= TIME_LCDFADEOUT * 1000) {
			LCDcurrentPW = sinMap(mil_delta, TIME_LCDFADEOUT * 1000, 0, LCDBRI_MIN);
			analogWrite(PIN_LCDBRI, LCDcurrentPW);
		}
		else
			setLCDMinLight();
	}


	//** after some time without user input, dim LCD
	if (state == STATE_RUN && LCDBacklightMode == LCDSTATE_MAX && millis() - mil_onAction > TIME_LCDFADEAFTER * 1000)
		startLCDFadeOut();

#pragma endregion



	/////////////////////////////////////////////////////////////////////////////////////
	// feedback to maintain LDR current
#pragma region loop_feedback
	if ((millis() - mil_onSetLDR > TIME_DACSETTLE) && state == STATE_RUN && volume < VOL_MAX_STEP && volume > 1) {

		// LSE
		word sample = getILSE();
		deltaSample = int(dataL[volume - 2].i_SE & 32767) - sample;
		deltaPW = deltaSample > 0 ? 1 : -1;
		if (deltaPW * deltaSample > abs(deltaPW * getDeltaI(dataL[volume - 2].i_SE) - deltaSample) && dataL[volume - 2].pw_SE > 0 && dataL[volume - 2].pw_SE < 255) {
			dataL[volume - 2].pw_SE += deltaPW;
			setLSE(dataL[volume - 2].pw_SE);
		}


		// RSE
		sample = getIRSE();
		deltaSample = int(dataR[volume - 2].i_SE & 32767) - sample;
		deltaPW = deltaSample > 0 ? 1 : -1;
		if (deltaPW * deltaSample > abs(deltaPW * getDeltaI(dataR[volume - 2].i_SE) - deltaSample) && dataR[volume - 2].pw_SE > 0 && dataR[volume - 2].pw_SE < 255) {
			dataR[volume - 2].pw_SE += deltaPW;
			setRSE(dataR[volume - 2].pw_SE);
		}


		// LSH
		sample = getILSH();
		deltaSample = int(dataL[volume - 2].i_SH & 32767) - sample;
		deltaPW = deltaSample > 0 ? 1 : -1;
		if (deltaPW * deltaSample > abs(deltaPW * getDeltaI(dataL[volume - 2].i_SH) - deltaSample) && dataL[volume - 2].pw_SH > 0 && dataL[volume - 2].pw_SH < 255) {
			dataL[volume - 2].pw_SH += deltaPW;
			setLSH(dataL[volume - 2].pw_SH);
		}


		// RSH
		sample = getIRSH();
		deltaSample = int(dataR[volume - 2].i_SH & 32767) - sample;
		deltaPW = deltaSample > 0 ? 1 : -1;
		if (deltaPW * deltaSample > abs(deltaPW * getDeltaI(dataR[volume - 2].i_SH) - deltaSample) && dataR[volume - 2].pw_SH > 0 && dataR[volume - 2].pw_SH < 255) {
			dataR[volume - 2].pw_SH += deltaPW;
			setRSH(dataR[volume - 2].pw_SH);
		}

		mil_onSetLDR = millis();
	}
#pragma endregion


}
#pragma endregion


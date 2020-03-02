#ifndef _ctlMenu_
#define _ctlMenu_
#include "MenuManager.h"
#include <avr/pgmspace.h>

/*

Generated using LCD Menu Builder at https://lcd-menu-bulder.cohesivecomputing.co.uk/
For more information, visit https://www.cohesivecomputing.co.uk/hackatronics/arduino-lcd-menu-library/

All our hackatronics projects are free for personal use. If you find them helpful or useful, please consider
making a small donation to our hackatronics fund using the donate buttons on our web pages. Thank you.
		
*/

enum ctlMenuCommandId
{
  mnuCmdBack = 0,
  mnuCmdINPUT_MENU,
  mnuCmdINPUT1_MENU,
  mnuCmdINPUT1_NAME,
  mnuCmdINPUT1_MODE,
  mnuCmdINPUT1_VOL_STEPS,
  mnuCmdINPUT1_MIN_dB,
  mnuCmdINPUT1_MAX_dB,
  mnuCmdINPUT1_MAX_START_VOL_STEP,
  mnuCmdINPUT2_MENU,
  mnuCmdINPUT2_NAME,
  mnuCmdINPUT2_MODE,
  mnuCmdINPUT2_VOL_STEPS,
  mnuCmdINPUT2_MIN_dB,
  mnuCmdINPUT2_MAX_dB,
  mnuCmdINPUT2_MAX_START_VOL_STEP,
  mnuCmdINPUT3_MENU,
  mnuCmdINPUT3_NAME,
  mnuCmdINPUT3_MODE,
  mnuCmdINPUT3_VOL_STEPS,
  mnuCmdINPUT3_MIN_dB,
  mnuCmdINPUT3_MAX_dB,
  mnuCmdINPUT3_MAX_START_VOL_STEP,
  mnuCmdINPUT4_MENU,
  mnuCmdINPUT4_NAME,
  mnuCmdINPUT4_MODE,
  mnuCmdINPUT4_VOL_STEPS,
  mnuCmdINPUT4_MIN_dB,
  mnuCmdINPUT4_MAX_dB,
  mnuCmdINPUT4_MAX_START_VOL_STEP,
  mnuCmdINPUT5_MENU,
  mnuCmdINPUT5_NAME,
  mnuCmdINPUT5_MODE,
  mnuCmdINPUT5_VOL_STEPS,
  mnuCmdINPUT5_MIN_dB,
  mnuCmdINPUT5_MAX_dB,
  mnuCmdINPUT5_MAX_START_VOL_STEP,
  mnuCmdINPUT6_MENU,
  mnuCmdINPUT6_NAME,
  mnuCmdINPUT6_MODE,
  mnuCmdINPUT6_VOL_STEPS,
  mnuCmdINPUT6_MIN_dB,
  mnuCmdINPUT6_MAX_dB,
  mnuCmdINPUT6_MAX_START_VOL_STEP,
  mnuCmdIR_MENU,
  mnuCmdIR_ONOFF,
  mnuCmdIR_UP,
  mnuCmdIR_DOWN,
  mnuCmdIR_REPEAT,
  mnuCmdIR_LEFT,
  mnuCmdIR_RIGHT,
  mnuCmdIR_SELECT,
  mnuCmdIR_BACK,
  mnuCmdIR_MUTE,
  mnuCmdIR_PREV,
  mnuCmdIR_1,
  mnuCmdIR_2,
  mnuCmdIR_3,
  mnuCmdIR_4,
  mnuCmdIR_5,
  mnuCmdIR_6,
  mnuCmdPWR_CTL_MENU,
  mnuCmdVOL_MEMORY,
  mnuCmdTRIG1_MENU,
  mnuCmdTRIGGER1_ACTIVE,
  mnuCmdTRIGGER1_TYPE,
  mnuCmdTRIGGER1_MODE,
  mnuCmdTRIGGER1_ON_DELAY,
  mnuCmdTRIGGER1_TEMP,
  mnuCmdTRIG2_MENU,
  mnuCmdTRIGGER2_ACTIVE,
  mnuCmdTRIGGER2_TYPE,
  mnuCmdTRIGGER2_MODE,
  mnuCmdTRIGGER2_ON_DELAY,
  mnuCmdTRIGGER2_TEMP,
  mnuCmdTRIGGER_INACT_TIMER,
  mnuCmdDISP_MENU,
  mnuCmdDISP_SAVER_ACTIVE,
  mnuCmdDISP_ON_LEVEL,
  mnuCmdDISP_DIM_LEVEL,
  mnuCmdDISP_DIM_TIMEOUT,
  mnuCmdDISP_VOL,
  mnuCmdDISP_INPUT,
  mnuCmdDISP_TEMP1,
  mnuCmdDISP_TEMP2,
  mnuCmdABOUT,
  mnuCmdRESET_MENU,
  mnuCmdRESET_NOW
};

PROGMEM const char ctlMenu_back[] = "Back";
PROGMEM const char ctlMenu_exit[] = "Exit";

PROGMEM const char ctlMenu_1_1_1[] = "Name";
PROGMEM const char ctlMenu_1_1_2[] = "Mode";
PROGMEM const char ctlMenu_1_1_3[] = "Volume steps";
PROGMEM const char ctlMenu_1_1_4[] = "Min. volume";
PROGMEM const char ctlMenu_1_1_5[] = "Max. volume";
PROGMEM const char ctlMenu_1_1_6[] = "Max start vol";
PROGMEM const MenuItem ctlMenu_List_1_1[] = {{mnuCmdINPUT1_NAME, ctlMenu_1_1_1}, {mnuCmdINPUT1_MODE, ctlMenu_1_1_2}, {mnuCmdINPUT1_VOL_STEPS, ctlMenu_1_1_3}, {mnuCmdINPUT1_MIN_dB, ctlMenu_1_1_4}, {mnuCmdINPUT1_MAX_dB, ctlMenu_1_1_5}, {mnuCmdINPUT1_MAX_START_VOL_STEP, ctlMenu_1_1_6}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1_2_1[] = "Name";
PROGMEM const char ctlMenu_1_2_2[] = "Mode";
PROGMEM const char ctlMenu_1_2_3[] = "Volume steps";
PROGMEM const char ctlMenu_1_2_4[] = "Min. volume";
PROGMEM const char ctlMenu_1_2_5[] = "Max. volume";
PROGMEM const char ctlMenu_1_2_6[] = "Max start vol";
PROGMEM const MenuItem ctlMenu_List_1_2[] = {{mnuCmdINPUT2_NAME, ctlMenu_1_2_1}, {mnuCmdINPUT2_MODE, ctlMenu_1_2_2}, {mnuCmdINPUT2_VOL_STEPS, ctlMenu_1_2_3}, {mnuCmdINPUT2_MIN_dB, ctlMenu_1_2_4}, {mnuCmdINPUT2_MAX_dB, ctlMenu_1_2_5}, {mnuCmdINPUT2_MAX_START_VOL_STEP, ctlMenu_1_2_6}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1_3_1[] = "Name";
PROGMEM const char ctlMenu_1_3_2[] = "Mode";
PROGMEM const char ctlMenu_1_3_3[] = "Volume steps";
PROGMEM const char ctlMenu_1_3_4[] = "Min. volume";
PROGMEM const char ctlMenu_1_3_5[] = "Max. volume";
PROGMEM const char ctlMenu_1_3_6[] = "Max start vol";
PROGMEM const MenuItem ctlMenu_List_1_3[] = {{mnuCmdINPUT3_NAME, ctlMenu_1_3_1}, {mnuCmdINPUT3_MODE, ctlMenu_1_3_2}, {mnuCmdINPUT3_VOL_STEPS, ctlMenu_1_3_3}, {mnuCmdINPUT3_MIN_dB, ctlMenu_1_3_4}, {mnuCmdINPUT3_MAX_dB, ctlMenu_1_3_5}, {mnuCmdINPUT3_MAX_START_VOL_STEP, ctlMenu_1_3_6}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1_4_1[] = "Name";
PROGMEM const char ctlMenu_1_4_2[] = "Mode";
PROGMEM const char ctlMenu_1_4_3[] = "Volume steps";
PROGMEM const char ctlMenu_1_4_4[] = "Min. volume";
PROGMEM const char ctlMenu_1_4_5[] = "Max. volume";
PROGMEM const char ctlMenu_1_4_6[] = "Max start vol";
PROGMEM const MenuItem ctlMenu_List_1_4[] = {{mnuCmdINPUT4_NAME, ctlMenu_1_4_1}, {mnuCmdINPUT4_MODE, ctlMenu_1_4_2}, {mnuCmdINPUT4_VOL_STEPS, ctlMenu_1_4_3}, {mnuCmdINPUT4_MIN_dB, ctlMenu_1_4_4}, {mnuCmdINPUT4_MAX_dB, ctlMenu_1_4_5}, {mnuCmdINPUT4_MAX_START_VOL_STEP, ctlMenu_1_4_6}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1_5_1[] = "Name";
PROGMEM const char ctlMenu_1_5_2[] = "Mode";
PROGMEM const char ctlMenu_1_5_3[] = "Volume steps";
PROGMEM const char ctlMenu_1_5_4[] = "Min. volume";
PROGMEM const char ctlMenu_1_5_5[] = "Max. volume";
PROGMEM const char ctlMenu_1_5_6[] = "Max start vol";
PROGMEM const MenuItem ctlMenu_List_1_5[] = {{mnuCmdINPUT5_NAME, ctlMenu_1_5_1}, {mnuCmdINPUT5_MODE, ctlMenu_1_5_2}, {mnuCmdINPUT5_VOL_STEPS, ctlMenu_1_5_3}, {mnuCmdINPUT5_MIN_dB, ctlMenu_1_5_4}, {mnuCmdINPUT5_MAX_dB, ctlMenu_1_5_5}, {mnuCmdINPUT5_MAX_START_VOL_STEP, ctlMenu_1_5_6}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1_6_1[] = "Name";
PROGMEM const char ctlMenu_1_6_2[] = "Mode";
PROGMEM const char ctlMenu_1_6_3[] = "Volume steps";
PROGMEM const char ctlMenu_1_6_4[] = "Min. volume";
PROGMEM const char ctlMenu_1_6_5[] = "Max. volume";
PROGMEM const char ctlMenu_1_6_6[] = "Max start vol";
PROGMEM const MenuItem ctlMenu_List_1_6[] = {{mnuCmdINPUT6_NAME, ctlMenu_1_6_1}, {mnuCmdINPUT6_MODE, ctlMenu_1_6_2}, {mnuCmdINPUT6_VOL_STEPS, ctlMenu_1_6_3}, {mnuCmdINPUT6_MIN_dB, ctlMenu_1_6_4}, {mnuCmdINPUT6_MAX_dB, ctlMenu_1_6_5}, {mnuCmdINPUT6_MAX_START_VOL_STEP, ctlMenu_1_6_6}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_3_2_1[] = "Active";
PROGMEM const char ctlMenu_3_2_2[] = "Moment./Latch";
PROGMEM const char ctlMenu_3_2_3[] = "Std./SmartON";
PROGMEM const char ctlMenu_3_2_4[] = "On delay";
PROGMEM const char ctlMenu_3_2_5[] = "Temp Ctrl";
PROGMEM const MenuItem ctlMenu_List_3_2[] = {{mnuCmdTRIGGER1_ACTIVE, ctlMenu_3_2_1}, {mnuCmdTRIGGER1_TYPE, ctlMenu_3_2_2}, {mnuCmdTRIGGER1_MODE, ctlMenu_3_2_3}, {mnuCmdTRIGGER1_ON_DELAY, ctlMenu_3_2_4}, {mnuCmdTRIGGER1_TEMP, ctlMenu_3_2_5}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_3_3_1[] = "Active";
PROGMEM const char ctlMenu_3_3_2[] = "Moment./Latch";
PROGMEM const char ctlMenu_3_3_3[] = "Std./SmartON";
PROGMEM const char ctlMenu_3_3_4[] = "On Delay";
PROGMEM const char ctlMenu_3_3_5[] = "Temp Ctrl";
PROGMEM const MenuItem ctlMenu_List_3_3[] = {{mnuCmdTRIGGER2_ACTIVE, ctlMenu_3_3_1}, {mnuCmdTRIGGER2_TYPE, ctlMenu_3_3_2}, {mnuCmdTRIGGER2_MODE, ctlMenu_3_3_3}, {mnuCmdTRIGGER2_ON_DELAY, ctlMenu_3_3_4}, {mnuCmdTRIGGER2_TEMP, ctlMenu_3_3_5}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1_1[] = "Input 1";
PROGMEM const char ctlMenu_1_2[] = "Input 2";
PROGMEM const char ctlMenu_1_3[] = "Input 3";
PROGMEM const char ctlMenu_1_4[] = "Input 4";
PROGMEM const char ctlMenu_1_5[] = "Input 5";
PROGMEM const char ctlMenu_1_6[] = "Input 6";
PROGMEM const MenuItem ctlMenu_List_1[] = {{mnuCmdINPUT1_MENU, ctlMenu_1_1, ctlMenu_List_1_1, menuCount(ctlMenu_List_1_1)}, {mnuCmdINPUT2_MENU, ctlMenu_1_2, ctlMenu_List_1_2, menuCount(ctlMenu_List_1_2)}, {mnuCmdINPUT3_MENU, ctlMenu_1_3, ctlMenu_List_1_3, menuCount(ctlMenu_List_1_3)}, {mnuCmdINPUT4_MENU, ctlMenu_1_4, ctlMenu_List_1_4, menuCount(ctlMenu_List_1_4)}, {mnuCmdINPUT5_MENU, ctlMenu_1_5, ctlMenu_List_1_5, menuCount(ctlMenu_List_1_5)}, {mnuCmdINPUT6_MENU, ctlMenu_1_6, ctlMenu_List_1_6, menuCount(ctlMenu_List_1_6)}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_2_1[] = "On/Off";
PROGMEM const char ctlMenu_2_2[] = "Up";
PROGMEM const char ctlMenu_2_3[] = "Down";
PROGMEM const char ctlMenu_2_4[] = "Repeat";
PROGMEM const char ctlMenu_2_5[] = "Left";
PROGMEM const char ctlMenu_2_6[] = "Right";
PROGMEM const char ctlMenu_2_7[] = "Select";
PROGMEM const char ctlMenu_2_8[] = "Back";
PROGMEM const char ctlMenu_2_9[] = "Mute";
PROGMEM const char ctlMenu_2_10[] = "Previous";
PROGMEM const char ctlMenu_2_11[] = "1";
PROGMEM const char ctlMenu_2_12[] = "2";
PROGMEM const char ctlMenu_2_13[] = "3";
PROGMEM const char ctlMenu_2_14[] = "4";
PROGMEM const char ctlMenu_2_15[] = "5";
PROGMEM const char ctlMenu_2_16[] = "6";
PROGMEM const MenuItem ctlMenu_List_2[] = {{mnuCmdIR_ONOFF, ctlMenu_2_1}, {mnuCmdIR_UP, ctlMenu_2_2}, {mnuCmdIR_DOWN, ctlMenu_2_3}, {mnuCmdIR_REPEAT, ctlMenu_2_4}, {mnuCmdIR_LEFT, ctlMenu_2_5}, {mnuCmdIR_RIGHT, ctlMenu_2_6}, {mnuCmdIR_SELECT, ctlMenu_2_7}, {mnuCmdIR_BACK, ctlMenu_2_8}, {mnuCmdIR_MUTE, ctlMenu_2_9}, {mnuCmdIR_PREV, ctlMenu_2_10}, {mnuCmdIR_1, ctlMenu_2_11}, {mnuCmdIR_2, ctlMenu_2_12}, {mnuCmdIR_3, ctlMenu_2_13}, {mnuCmdIR_4, ctlMenu_2_14}, {mnuCmdIR_5, ctlMenu_2_15}, {mnuCmdIR_6, ctlMenu_2_16}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_3_1[] = "Save Inp./Vol.";
PROGMEM const char ctlMenu_3_2[] = "Trigger 1";
PROGMEM const char ctlMenu_3_3[] = "Trigger 2";
PROGMEM const char ctlMenu_3_4[] = "Standby Timer";
PROGMEM const MenuItem ctlMenu_List_3[] = {{mnuCmdVOL_MEMORY, ctlMenu_3_1}, {mnuCmdTRIG1_MENU, ctlMenu_3_2, ctlMenu_List_3_2, menuCount(ctlMenu_List_3_2)}, {mnuCmdTRIG2_MENU, ctlMenu_3_3, ctlMenu_List_3_3, menuCount(ctlMenu_List_3_3)}, {mnuCmdTRIGGER_INACT_TIMER, ctlMenu_3_4}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_4_1[] = "Screen saver";
PROGMEM const char ctlMenu_4_2[] = "On Level";
PROGMEM const char ctlMenu_4_3[] = "Dim Level";
PROGMEM const char ctlMenu_4_4[] = "Timeout";
PROGMEM const char ctlMenu_4_5[] = "Show Volume";
PROGMEM const char ctlMenu_4_6[] = "Show Input";
PROGMEM const char ctlMenu_4_7[] = "Show Temp 1";
PROGMEM const char ctlMenu_4_8[] = "Show Temp 2";
PROGMEM const MenuItem ctlMenu_List_4[] = {{mnuCmdDISP_SAVER_ACTIVE, ctlMenu_4_1}, {mnuCmdDISP_ON_LEVEL, ctlMenu_4_2}, {mnuCmdDISP_DIM_LEVEL, ctlMenu_4_3}, {mnuCmdDISP_DIM_TIMEOUT, ctlMenu_4_4}, {mnuCmdDISP_VOL, ctlMenu_4_5}, {mnuCmdDISP_INPUT, ctlMenu_4_6}, {mnuCmdDISP_TEMP1, ctlMenu_4_7}, {mnuCmdDISP_TEMP2, ctlMenu_4_8}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_6_1[] = "Reset settings";
PROGMEM const MenuItem ctlMenu_List_6[] = {{mnuCmdRESET_NOW, ctlMenu_6_1}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1[] = "Inputs";
PROGMEM const char ctlMenu_2[] = "Learn IR";
PROGMEM const char ctlMenu_3[] = "Power Settings";
PROGMEM const char ctlMenu_4[] = "Display";
PROGMEM const char ctlMenu_5[] = "About";
PROGMEM const char ctlMenu_6[] = "Reset";
PROGMEM const MenuItem ctlMenu_Root[] = {{mnuCmdINPUT_MENU, ctlMenu_1, ctlMenu_List_1, menuCount(ctlMenu_List_1)}, {mnuCmdIR_MENU, ctlMenu_2, ctlMenu_List_2, menuCount(ctlMenu_List_2)}, {mnuCmdPWR_CTL_MENU, ctlMenu_3, ctlMenu_List_3, menuCount(ctlMenu_List_3)}, {mnuCmdDISP_MENU, ctlMenu_4, ctlMenu_List_4, menuCount(ctlMenu_List_4)}, {mnuCmdABOUT, ctlMenu_5}, {mnuCmdRESET_MENU, ctlMenu_6, ctlMenu_List_6, menuCount(ctlMenu_List_6)}, {mnuCmdBack, ctlMenu_exit}};

#endif
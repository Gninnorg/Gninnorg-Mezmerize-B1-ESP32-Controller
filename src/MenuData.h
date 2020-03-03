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
  mnuCmdVOLUME_MENU,
  mnuCmdVOL_STEPS,
  mnuCmdMIN_VOL,
  mnuCmdMAX_VOL,
  mnuCmdMAX_START_VOL,
  mnuCmdMUTE_LVL,
  mnuCmdSTORE_LVL,
  mnuCmdINPUT_MENU,
  mnuCmdINPUT1_MENU,
  mnuCmdINPUT1_ACTIVE,
  mnuCmdINPUT1_NAME,
  mnuCmdINPUT1_MAX_VOL,
  mnuCmdINPUT1_MIN_VOL,
  mnuCmdINPUT2_MENU,
  mnuCmdINPUT2_ACTIVE,
  mnuCmdINPUT2_NAME,
  mnuCmdINPUT2_MAX_VOL,
  mnuCmdINPUT2_MIN_VOL,
  mnuCmdINPUT3_MENU,
  mnuCmdINPUT3_ACTIVE,
  mnuCmdINPUT3_NAME,
  mnuCmdINPUT3_MAX_VOL,
  mnuCmdINPUT3_MIN_VOL,
  mnuCmdINPUT4_MENU,
  mnuCmdINPUT4_ACTIVE,
  mnuCmdINPUT4_NAME,
  mnuCmdINPUT4_MAX_VOL,
  mnuCmdINPUT4_MIN_VOL,
  mnuCmdINPUT5_MENU,
  mnuCmdINPUT5_ACTIVE,
  mnuCmdINPUT5_NAME,
  mnuCmdINPUT5_MAX_VOL,
  mnuCmdINPUT5_MIN_VOL,
  mnuCmdINPUT6_MENU,
  mnuCmdINPUT6_ACTIVE,
  mnuCmdINPUT6_NAME,
  mnuCmdINPUT6_MAX_VOL,
  mnuCmdINPUT6_MIN_VOL,
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
  mnuCmdDISP_INPUT,
  mnuCmdDISP_TEMP1,
  mnuCmdDISP_TEMP2,
  mnuCmdABOUT,
  mnuCmdRESET_MENU,
  mnuCmdRESET_NOW
};

PROGMEM const char ctlMenu_back[] = "Back";
PROGMEM const char ctlMenu_exit[] = "Exit";

PROGMEM const char ctlMenu_2_1_1[] = "Active";
PROGMEM const char ctlMenu_2_1_2[] = "Name";
PROGMEM const char ctlMenu_2_1_3[] = "Max. volume";
PROGMEM const char ctlMenu_2_1_4[] = "Min. volume";
PROGMEM const MenuItem ctlMenu_List_2_1[] = {{mnuCmdINPUT1_ACTIVE, ctlMenu_2_1_1}, {mnuCmdINPUT1_NAME, ctlMenu_2_1_2}, {mnuCmdINPUT1_MAX_VOL, ctlMenu_2_1_3}, {mnuCmdINPUT1_MIN_VOL, ctlMenu_2_1_4}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_2_2_1[] = "Active";
PROGMEM const char ctlMenu_2_2_2[] = "Name";
PROGMEM const char ctlMenu_2_2_3[] = "Max. volume";
PROGMEM const char ctlMenu_2_2_4[] = "Min. volume";
PROGMEM const MenuItem ctlMenu_List_2_2[] = {{mnuCmdINPUT2_ACTIVE, ctlMenu_2_2_1}, {mnuCmdINPUT2_NAME, ctlMenu_2_2_2}, {mnuCmdINPUT2_MAX_VOL, ctlMenu_2_2_3}, {mnuCmdINPUT2_MIN_VOL, ctlMenu_2_2_4}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_2_3_1[] = "Active";
PROGMEM const char ctlMenu_2_3_2[] = "Name";
PROGMEM const char ctlMenu_2_3_3[] = "Max. volume";
PROGMEM const char ctlMenu_2_3_4[] = "Min. volume";
PROGMEM const MenuItem ctlMenu_List_2_3[] = {{mnuCmdINPUT3_ACTIVE, ctlMenu_2_3_1}, {mnuCmdINPUT3_NAME, ctlMenu_2_3_2}, {mnuCmdINPUT3_MAX_VOL, ctlMenu_2_3_3}, {mnuCmdINPUT3_MIN_VOL, ctlMenu_2_3_4}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_2_4_1[] = "Active";
PROGMEM const char ctlMenu_2_4_2[] = "Name";
PROGMEM const char ctlMenu_2_4_3[] = "Max. volume";
PROGMEM const char ctlMenu_2_4_4[] = "Min. volume";
PROGMEM const MenuItem ctlMenu_List_2_4[] = {{mnuCmdINPUT4_ACTIVE, ctlMenu_2_4_1}, {mnuCmdINPUT4_NAME, ctlMenu_2_4_2}, {mnuCmdINPUT4_MAX_VOL, ctlMenu_2_4_3}, {mnuCmdINPUT4_MIN_VOL, ctlMenu_2_4_4}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_2_5_1[] = "Active";
PROGMEM const char ctlMenu_2_5_2[] = "Name";
PROGMEM const char ctlMenu_2_5_3[] = "Max. volume";
PROGMEM const char ctlMenu_2_5_4[] = "Min. volume";
PROGMEM const MenuItem ctlMenu_List_2_5[] = {{mnuCmdINPUT5_ACTIVE, ctlMenu_2_5_1}, {mnuCmdINPUT5_NAME, ctlMenu_2_5_2}, {mnuCmdINPUT5_MAX_VOL, ctlMenu_2_5_3}, {mnuCmdINPUT5_MIN_VOL, ctlMenu_2_5_4}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_2_6_1[] = "Active";
PROGMEM const char ctlMenu_2_6_2[] = "Name";
PROGMEM const char ctlMenu_2_6_3[] = "Max. volume";
PROGMEM const char ctlMenu_2_6_4[] = "Min. volume";
PROGMEM const MenuItem ctlMenu_List_2_6[] = {{mnuCmdINPUT6_ACTIVE, ctlMenu_2_6_1}, {mnuCmdINPUT6_NAME, ctlMenu_2_6_2}, {mnuCmdINPUT6_MAX_VOL, ctlMenu_2_6_3}, {mnuCmdINPUT6_MIN_VOL, ctlMenu_2_6_4}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_4_1_1[] = "Active";
PROGMEM const char ctlMenu_4_1_2[] = "Moment./Latch";
PROGMEM const char ctlMenu_4_1_3[] = "Std./SmartON";
PROGMEM const char ctlMenu_4_1_4[] = "On delay";
PROGMEM const char ctlMenu_4_1_5[] = "Temp Ctrl";
PROGMEM const MenuItem ctlMenu_List_4_1[] = {{mnuCmdTRIGGER1_ACTIVE, ctlMenu_4_1_1}, {mnuCmdTRIGGER1_TYPE, ctlMenu_4_1_2}, {mnuCmdTRIGGER1_MODE, ctlMenu_4_1_3}, {mnuCmdTRIGGER1_ON_DELAY, ctlMenu_4_1_4}, {mnuCmdTRIGGER1_TEMP, ctlMenu_4_1_5}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_4_2_1[] = "Active";
PROGMEM const char ctlMenu_4_2_2[] = "Moment./Latch";
PROGMEM const char ctlMenu_4_2_3[] = "Std./SmartON";
PROGMEM const char ctlMenu_4_2_4[] = "On Delay";
PROGMEM const char ctlMenu_4_2_5[] = "Temp Ctrl";
PROGMEM const MenuItem ctlMenu_List_4_2[] = {{mnuCmdTRIGGER2_ACTIVE, ctlMenu_4_2_1}, {mnuCmdTRIGGER2_TYPE, ctlMenu_4_2_2}, {mnuCmdTRIGGER2_MODE, ctlMenu_4_2_3}, {mnuCmdTRIGGER2_ON_DELAY, ctlMenu_4_2_4}, {mnuCmdTRIGGER2_TEMP, ctlMenu_4_2_5}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1_1[] = "Volume steps";
PROGMEM const char ctlMenu_1_2[] = "Min. volume";
PROGMEM const char ctlMenu_1_3[] = "Max. volume";
PROGMEM const char ctlMenu_1_4[] = "Max start vol";
PROGMEM const char ctlMenu_1_5[] = "Mute level";
PROGMEM const char ctlMenu_1_6[] = "Vol. memory";
PROGMEM const MenuItem ctlMenu_List_1[] = {{mnuCmdVOL_STEPS, ctlMenu_1_1}, {mnuCmdMIN_VOL, ctlMenu_1_2}, {mnuCmdMAX_VOL, ctlMenu_1_3}, {mnuCmdMAX_START_VOL, ctlMenu_1_4}, {mnuCmdMUTE_LVL, ctlMenu_1_5}, {mnuCmdSTORE_LVL, ctlMenu_1_6}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_2_1[] = "Input 1";
PROGMEM const char ctlMenu_2_2[] = "Input 2";
PROGMEM const char ctlMenu_2_3[] = "Input 3";
PROGMEM const char ctlMenu_2_4[] = "Input 4";
PROGMEM const char ctlMenu_2_5[] = "Input 5";
PROGMEM const char ctlMenu_2_6[] = "Input 6";
PROGMEM const MenuItem ctlMenu_List_2[] = {{mnuCmdINPUT1_MENU, ctlMenu_2_1, ctlMenu_List_2_1, menuCount(ctlMenu_List_2_1)}, {mnuCmdINPUT2_MENU, ctlMenu_2_2, ctlMenu_List_2_2, menuCount(ctlMenu_List_2_2)}, {mnuCmdINPUT3_MENU, ctlMenu_2_3, ctlMenu_List_2_3, menuCount(ctlMenu_List_2_3)}, {mnuCmdINPUT4_MENU, ctlMenu_2_4, ctlMenu_List_2_4, menuCount(ctlMenu_List_2_4)}, {mnuCmdINPUT5_MENU, ctlMenu_2_5, ctlMenu_List_2_5, menuCount(ctlMenu_List_2_5)}, {mnuCmdINPUT6_MENU, ctlMenu_2_6, ctlMenu_List_2_6, menuCount(ctlMenu_List_2_6)}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_3_1[] = "On/Off";
PROGMEM const char ctlMenu_3_2[] = "Up";
PROGMEM const char ctlMenu_3_3[] = "Down";
PROGMEM const char ctlMenu_3_4[] = "Repeat";
PROGMEM const char ctlMenu_3_5[] = "Left";
PROGMEM const char ctlMenu_3_6[] = "Right";
PROGMEM const char ctlMenu_3_7[] = "Select";
PROGMEM const char ctlMenu_3_8[] = "Back";
PROGMEM const char ctlMenu_3_9[] = "Mute";
PROGMEM const char ctlMenu_3_10[] = "Previous";
PROGMEM const char ctlMenu_3_11[] = "1";
PROGMEM const char ctlMenu_3_12[] = "2";
PROGMEM const char ctlMenu_3_13[] = "3";
PROGMEM const char ctlMenu_3_14[] = "4";
PROGMEM const char ctlMenu_3_15[] = "5";
PROGMEM const char ctlMenu_3_16[] = "6";
PROGMEM const MenuItem ctlMenu_List_3[] = {{mnuCmdIR_ONOFF, ctlMenu_3_1}, {mnuCmdIR_UP, ctlMenu_3_2}, {mnuCmdIR_DOWN, ctlMenu_3_3}, {mnuCmdIR_REPEAT, ctlMenu_3_4}, {mnuCmdIR_LEFT, ctlMenu_3_5}, {mnuCmdIR_RIGHT, ctlMenu_3_6}, {mnuCmdIR_SELECT, ctlMenu_3_7}, {mnuCmdIR_BACK, ctlMenu_3_8}, {mnuCmdIR_MUTE, ctlMenu_3_9}, {mnuCmdIR_PREV, ctlMenu_3_10}, {mnuCmdIR_1, ctlMenu_3_11}, {mnuCmdIR_2, ctlMenu_3_12}, {mnuCmdIR_3, ctlMenu_3_13}, {mnuCmdIR_4, ctlMenu_3_14}, {mnuCmdIR_5, ctlMenu_3_15}, {mnuCmdIR_6, ctlMenu_3_16}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_4_1[] = "Trigger 1";
PROGMEM const char ctlMenu_4_2[] = "Trigger 2";
PROGMEM const char ctlMenu_4_3[] = "Standby Timer";
PROGMEM const MenuItem ctlMenu_List_4[] = {{mnuCmdTRIG1_MENU, ctlMenu_4_1, ctlMenu_List_4_1, menuCount(ctlMenu_List_4_1)}, {mnuCmdTRIG2_MENU, ctlMenu_4_2, ctlMenu_List_4_2, menuCount(ctlMenu_List_4_2)}, {mnuCmdTRIGGER_INACT_TIMER, ctlMenu_4_3}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_5_1[] = "Screen saver";
PROGMEM const char ctlMenu_5_2[] = "On Level";
PROGMEM const char ctlMenu_5_3[] = "Dim Level";
PROGMEM const char ctlMenu_5_4[] = "Timeout";
PROGMEM const char ctlMenu_5_5[] = "Show Input";
PROGMEM const char ctlMenu_5_6[] = "Show Temp 1";
PROGMEM const char ctlMenu_5_7[] = "Show Temp 2";
PROGMEM const MenuItem ctlMenu_List_5[] = {{mnuCmdDISP_SAVER_ACTIVE, ctlMenu_5_1}, {mnuCmdDISP_ON_LEVEL, ctlMenu_5_2}, {mnuCmdDISP_DIM_LEVEL, ctlMenu_5_3}, {mnuCmdDISP_DIM_TIMEOUT, ctlMenu_5_4}, {mnuCmdDISP_INPUT, ctlMenu_5_5}, {mnuCmdDISP_TEMP1, ctlMenu_5_6}, {mnuCmdDISP_TEMP2, ctlMenu_5_7}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_7_1[] = "Reset settings";
PROGMEM const MenuItem ctlMenu_List_7[] = {{mnuCmdRESET_NOW, ctlMenu_7_1}, {mnuCmdBack, ctlMenu_back}};

PROGMEM const char ctlMenu_1[] = "Volume";
PROGMEM const char ctlMenu_2[] = "Inputs";
PROGMEM const char ctlMenu_3[] = "Learn IR";
PROGMEM const char ctlMenu_4[] = "Triggers";
PROGMEM const char ctlMenu_5[] = "Display";
PROGMEM const char ctlMenu_6[] = "About";
PROGMEM const char ctlMenu_7[] = "Reset";
PROGMEM const MenuItem ctlMenu_Root[] = {{mnuCmdVOLUME_MENU, ctlMenu_1, ctlMenu_List_1, menuCount(ctlMenu_List_1)}, {mnuCmdINPUT_MENU, ctlMenu_2, ctlMenu_List_2, menuCount(ctlMenu_List_2)}, {mnuCmdIR_MENU, ctlMenu_3, ctlMenu_List_3, menuCount(ctlMenu_List_3)}, {mnuCmdPWR_CTL_MENU, ctlMenu_4, ctlMenu_List_4, menuCount(ctlMenu_List_4)}, {mnuCmdDISP_MENU, ctlMenu_5, ctlMenu_List_5, menuCount(ctlMenu_List_5)}, {mnuCmdABOUT, ctlMenu_6}, {mnuCmdRESET_MENU, ctlMenu_7, ctlMenu_List_7, menuCount(ctlMenu_List_7)}, {mnuCmdBack, ctlMenu_exit}};
#endif
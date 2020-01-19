/*
**
** Relay controller for MezmerizeB1Buffer
**
** Copyright (c) 2020 Carsten Grønning, Jan Abkjer Tofft
**
** Version 0.1
**
** PIN1 & PIN2 : Used for controlling amplifier triggers relays
** PIN3 - PIN8 : Used for controlling input relays
**                     
*/

#define NUM_OF_INPUTS 6

#include <Arduino.h>
#include <Adafruit_MCP23008.h>

class RelayController
{
public:
	
	RelayController();

	void begin();

	// Controlling input relays
	void setInput(uint8_t inputNmbr);
	void setInputName(uint8_t inputNmbr, char* inputName);
    uint8_t getInput();
	char* getInputName(uint8_t inputNmbr);
	void mute(boolean on);


    // Controlling triggers for amplifier.
	// Either use standard trigger 12 V or the alternate trigger, which uses pins to sense if amplifier is turned on
	void setAlternateTrigger(uint8_t inputRight, uint8_t inputLeft);
	void setStandardTrigger();
	void setTriggerOn();
	void SetTriggerOff();
	
private:
	Adafruit_MCP23008 	mcp;
	uint8_t				selectedInput;
	uint8_t				sensorRight;
	uint8_t				sensorLeft;
	boolean				standardTrigger;

	// Change number of inputs used (max=6) and input names below.
	uint8_t				numOfInputs = 6;
	char* 				inputName[6] = 		{"INPUT 1", "INPUT 2", "INPUT 3", "INPUT 4", "INPUT 5", "INPUT 6" };
};


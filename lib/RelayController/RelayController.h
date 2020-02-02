/*
**
** Relay controller for MezmerizeB1Buffer
**
** Copyright (c) 2020 Carsten Grønning, Jan Abkjer Tofft
**
** Version 0.1
**
** PIN6 & PIN7 : Used for controlling amplifier triggers relays
** PIN0 - PIN5 : Used for controlling input relays
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
	void setInputName(uint8_t inputNmbr, String name);
    uint8_t getInput();
	String getInputName(uint8_t inputNmbr);
	void mute(boolean on);


    // Controlling triggers for amplifier.
	// Either use standard trigger 12 V or the alternate trigger, which uses pins to sense if amplifier is turned on
	//void setAlternateTrigger(uint8_t inputRight = A2, uint8_t inputLeft = A3);
	void setStandardTrigger();
	void setTriggerOn();
	void SetTriggerOff();
	
private:
	Adafruit_MCP23008 	mcp;
	uint8_t				selectedInput = 9;
	uint8_t				sensorRight = A2;
	uint8_t				sensorLeft  = A3;
	boolean				standardTrigger;

	// Change number of inputs used (max=6) and input names below.
	uint8_t				numOfInputs = NUM_OF_INPUTS;
	String              inputName[6] = {"Input 1","Input 2","Input 3","Input 4","Input 5","Input 6"};
};


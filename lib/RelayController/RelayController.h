/*
  
  A description of the input selector

*/

#define NUM_OF_INPUTS 6

#include <Arduino.h>
#include <Adafruit_MCP23008.h>

class RelayController
{
public:
	
	// contextual data types.
	typedef uint8_t input_t;
	typedef boolean trigger_t;
	
	RelayController(input_t inputs=NUM_OF_INPUTS);

	void begin();

	void setInput(input_t inputNmbr);
	void setInputName(input_t inputNmbr, char* inputName);
	void setAlternateTrigger(uint8_t inputRight, uint8_t inputLeft);
	void setStandardTrigger();
	boolean setTriggerOn();
	void SetTrifferOff();
	
	input_t getInput();
	char* getInputName(input_t inputNmbr);

	void mute(boolean on);

private:
	input_t				selectedInput;
	input_t				numOfInputs;
	input_t				sensorRight;
	input_t				sensorLeft;
	boolean				standardTrigger;
	Adafruit_MCP23008 	mcp;
	trigger_t 			triggerType;
};


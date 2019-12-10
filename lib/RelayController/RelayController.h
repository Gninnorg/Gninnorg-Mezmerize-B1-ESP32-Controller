/*
  
  A description of the input selector

*/
#define TRIGGER_12V true   
#define TRIGGER_ALT false
#define NUM_OF_INPUTS 6

#include <Arduino.h>
#include <Adafruit_MCP23008.h>

class RelayController
{
public:
	
	// contextual data types.
	typedef uint8_t input_t;
	typedef boolean trigger_t;
	
	RelayController(input_t inputs=NUM_OF_INPUTS, trigger_t trigger=TRIGGER_12V);

	void begin();

	void setInput(input_t inputNmbr);
	void setInputName(input_t inputNmbr, char* inputName);
	
	input_t getInput();
	char* getInputName(input_t inputNmbr);

	void mute(boolean on);

private:
	input_t				selectedInput;
	input_t				numOfInputs;
	Adafruit_MCP23008 	mcp;
	trigger_t 			triggerType;
};


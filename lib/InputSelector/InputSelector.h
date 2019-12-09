/*
  
  A description of the input selector

*/

#include <Arduino.h>

class InputSelector
{
public:
	
	// contextual data types.
	typedef uint8_t pin_t;
	
	InputSelector(uint8_t i2cAddress);

	void begin();

	void setInput();
		
	void getInput();

	void getInputName();

private:
  void dummy();

private:
	address_t chip_address;
};


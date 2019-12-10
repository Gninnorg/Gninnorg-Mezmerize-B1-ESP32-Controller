/*
 
*/

#include "RelayController.h"
#include <Wire.h>

RelayController::RelayController(input_t inputs=NUM_OF_INPUTS, trigger_t trigger=TRIGGER_12V) 
{
    numOfInputs = inputs;
    triggerType = trigger;
}

void RelayController::begin()
{
    Wire.begin();
    mcp.begin();
    for (byte pin = 1; pin <= 8; pin++) 
    {
        mcp.pinMode(pin, OUTPUT);
        mcp.digitalWrite(pin, LOW);
    }
}

void RelayController::setInput(input_t inputNmbr)
{
    for (byte pin = 0; pin < numOfInputs; pin++) 
    {
        mcp.digitalWrite(pin, (inputNmbr == pin));
        selectedInput = inputNmbr;
    }
}

void RelayController::setInputName(input_t inputNmbr, char* inputName)
{

}

uint8_t RelayController::getInput()
{
    return selectedInput;
}

char*  RelayController::getInputName(input_t inputNmbr)
{
}

void RelayController::mute(boolean on)
{
    mcp.digitalWrite(selectedInput, on == false);
}
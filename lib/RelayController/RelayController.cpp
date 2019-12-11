/*
 
*/

#include "RelayController.h"
#include <Wire.h>

RelayController::RelayController(input_t inputs=NUM_OF_INPUTS) 
{
    numOfInputs = inputs;
    setStandardTrigger();
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
    for (byte pin = 3; pin < (numOfInputs + 2); pin++) 
    {
        mcp.digitalWrite(pin, (inputNmbr == pin));
        selectedInput = inputNmbr;
    }
}

void RelayController::setInputName(input_t inputNmbr, char* inputName)
{

}

void RelayController::setAlternateTrigger(uint8_t inputRight, uint8_t inputLeft)
{
    sensorRight = inputRight;
    sensorLeft = inputLeft;
    standardTrigger = false;
}

void RelayController::setStandardTrigger()
{
    sensorRight = 0;
    sensorLeft = 0;
    standardTrigger = true;
}

void RelayController::setTriggerOn()
{
    if (standardTrigger) {
        mcp.digitalWrite(1,HIGH);
        mcp.digitalWrite(2, HIGH);
    } else {
        // Add logic to handle alternative trigger here
    }
}

void RelayController::SetTrifferOff()
{
    if (standardTrigger) {
        mcp.digitalWrite(1,LOW);
        mcp.digitalWrite(2, LOW);
    } else {
        // Add logic to handle alternative trigger here
    }
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
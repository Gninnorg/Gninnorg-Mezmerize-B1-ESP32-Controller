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

#include <RelayController.h>

RelayController::RelayController() 
{
     setStandardTrigger();
}

void RelayController::begin()
{
    mcp.begin();
    for (byte pin = 0; pin <= 7; pin++) 
    {
        mcp.pinMode(pin, OUTPUT);
    }
    Serial.println("started!");
    
}

void RelayController::setInput(uint8_t inputNmbr)
{
    for (byte pin = 0; pin < (numOfInputs - 1 ); pin++) 
    {
        mcp.digitalWrite(pin, (inputNmbr == pin));
        selectedInput = inputNmbr;
    }
}

void RelayController::setInputName(uint8_t inputNmbr, char* inputName)
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
        delay(3000);
        Serial.println("SetTrigger:Standard");
        mcp.digitalWrite(6, HIGH);
        mcp.digitalWrite(7, HIGH);
    } else {
        // Add logic to handle alternative trigger here
    }
}

void RelayController::SetTriggerOff()
{
    if (standardTrigger) {
        mcp.digitalWrite(6,LOW);
        mcp.digitalWrite(7, LOW);
    } else {
        // Add logic to handle alternative trigger here
    }
}


uint8_t RelayController::getInput()
{
    return selectedInput;
}

char*  RelayController::getInputName(uint8_t inputNmbr)
{
    return inputName[selectedInput-1];
}

void RelayController::mute(boolean on)
{
    mcp.digitalWrite(selectedInput, on == false);
}
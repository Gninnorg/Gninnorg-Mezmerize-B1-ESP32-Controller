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
    inputNmbr --;
    if (inputNmbr != selectedInput) {
        for (byte pin = 0; pin < (numOfInputs); pin++) 
        {
            mcp.digitalWrite(pin, (inputNmbr == pin));
            selectedInput = inputNmbr;
        }
    }
}

void RelayController::setInputName(uint8_t inputNmbr, String name)
{
    switch (inputNmbr)
    {
    case 1:
        input1 = name;
        break;
    case 2:
        input2 = name;
        break;
    case 3:
        input3 = name;
        break;
    case 4:
        input4 = name;
        break;
    case 5:
        input5 = name;
        break;
    case 6:
        input6 = name;
        break;
    default:
        break;
    }
}

/*void RelayController::setAlternateTrigger(uint8_t inputRight, uint8_t inputLeft)
{
    sensorRight = inputRight;
    sensorLeft = inputLeft;
    standardTrigger = false;
}*/

void RelayController::setStandardTrigger()
{
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
    return selectedInput+1;
}

String RelayController::getInputName(uint8_t inputNmbr)
{
    switch (inputNmbr)
    {
    case 1:
        return input1;
        break;
    case 2:
        return input2;
        break;
    case 3:
        return input3;
        break;
    case 4:
        return input4;
        break;
    case 5:
        return input5;
        break;
    case 6:
        return input6;
        break;
    default:
        return "";
        break;
    }
}

void RelayController::mute(boolean on)
{
    mcp.digitalWrite(selectedInput, on == false);
}
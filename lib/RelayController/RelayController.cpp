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
    // Start communication to MCP
    mcp.begin();

    // Defines all pins to OUTPUT and disable all relay 
    for (byte pin = 0; pin <= 7; pin++) 
    {
        mcp.pinMode(pin, OUTPUT);
        mcp.digitalWrite(pin, LOW);
    }
}

void RelayController::setInput(uint8_t inputNmbr)
{
    //Remap selected input til MCP pin
    uint8_t pin_sel = 8 - inputNmbr + 1;
    uint8_t pin_unsel = 8 - selectedInput + 1;
    
    //Unselect previous input relay
    mcp.digitalWrite(pin_unsel, LOW);

    //Select neew input and save the selected input relay
    mcp.digitalWrite(pin_sel, HIGH);
    selectedInput = pin_sel;
}

void RelayController::setInputName(uint8_t inputNmbr, String name)
{
    //Update name of input
    if (inputNmbr >= 1 && inputNmbr <= 6) inputName[inputNmbr] = name;
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
        mcp.digitalWrite(1, HIGH);
        mcp.digitalWrite(2, HIGH);
    } else {
        // Add logic to handle alternative trigger here
    }
}

void RelayController::SetTriggerOff()
{
    if (standardTrigger) {
        mcp.digitalWrite(1,LOW);
        mcp.digitalWrite(2,LOW);
    } else {
        // Add logic to handle alternative trigger here
    }
}


uint8_t RelayController::getInput()
{
    return selectedInput;
}

String RelayController::getInputName(uint8_t inputNmbr)
{
    if (inputNmbr >= 1 && inputNmbr <= 6) {
        return inputName[inputNmbr];
    } else return "";
}

void RelayController::mute(boolean on)
{
    mcp.digitalWrite(selectedInput, on == false);
}
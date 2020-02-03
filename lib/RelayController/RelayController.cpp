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
** PIN8 --> INPUT1 (8 - INPUT + 1)
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

void RelayController::setRelayOn(uint8_t Nmbr)
{
    //Remap selected input til MCP pin
    uint8_t pin_sel = 8 - Nmbr + 1;
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

// <TO DO: Should we have separate functions to call for the two relays?>
// standardTrigger is when the 12V trigger circuit is used
void RelayController::setStandardTrigger()
{
    standardTrigger = true;
}

// <TO DO: Should we have separate functions to call for the two relays?>
void RelayController::setTriggerOn()
{
    if (standardTrigger) {
        delay(3000);
        Serial.println("SetTrigger:Standard");
        mcp.digitalWrite(1, HIGH);
        mcp.digitalWrite(2, HIGH);
    } else {
        Serial.println("SetTrigger: Alternative");
        // Add logic to handle alternative trigger here
        // Measure NTC and LDR for one channel
        // If NTC > 100000 Ohms then the amp is off, so try to turn it on:
                setRelayOn(6);
                delay(100);
                setRelayOff(6);
        //      Measure NTC and LDR again
        //      If NTC is still > 100000 then the amplifier is still off = probably not connected to mains (display error and wait for the user to turn the power on and click on the encoder. Then try again)
        // else do nothing (the amp is already turned on)
        
        // Repeat the code above for the other channel
        setRelayOn(7);
        delay(100);
        setRelayOff(7);
    }
}

// <TO DO: Should we have separate functions to call for the two relays?>
void RelayController::SetTriggerOff()
{
    if (standardTrigger) {
        mcp.digitalWrite(1,LOW);
        mcp.digitalWrite(2,LOW);
    } else {
         Serial.println("SetTriggerOff: Alternative");
        // Add logic to handle alternative trigger here
        // Measure NTC and LDR for one channel
        // If NTC < 100000 Ohms then the amp is on, so turn if off:
                setRelayOff(6);
                delay(100);
                setRelayOff(6);
        // else the amplifier is already off so do nothing
        
        // Repeat the code above for the other channel
        setRelayOff(7);
        delay(100);
        setRelayOff(7);
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
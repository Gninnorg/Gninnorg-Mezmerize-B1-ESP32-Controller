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

void RelayController::setRelayOn(uint8_t Nmbr)
{
    Serial.print("Relay "); Serial.print(Nmbr); Serial.println(" is ON");
    mcp.digitalWrite(Nmbr, HIGH);
}

void RelayController::setRelayOff(uint8_t Nmbr)
{
    Serial.print("Relay "); Serial.print(Nmbr); Serial.println(" is OFF");
    mcp.digitalWrite(Nmbr, LOW);
}


void RelayController::setInput(uint8_t inputNmbr) // Input number from 1 to numOfInputs accepted
{
    inputNmbr --;
    if ((inputNmbr < numOfInputs) && (inputNmbr != selectedInput)) {
        // Turn all inputs off
        for (byte pin = 0; pin < numOfInputs; pin++) setRelayOff(pin);
            
        // Turn selected input on
        setRelayOn(inputNmbr);
        selectedInput = inputNmbr;
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
        Serial.println("SetTrigger: Standard");
        setRelayOn(6);
        setRelayOn(7); 
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
        Serial.println("SetTriggerOff: Standard");
        setRelayOff(6);
        setRelayOff(7); 
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
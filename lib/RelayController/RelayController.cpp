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
    mcp.digitalWrite(Nmbr, HIGH);
}

void RelayController::setRelayOff(uint8_t Nmbr)
{
    mcp.digitalWrite(Nmbr, LOW);
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
        if (getTemperature(A1) < 0)
        {
          setRelayOn(6);
          delay(200);
          if (getTemperature(A1) < 0)
            Serial.println("Check power to power amp L");
          else
            Serial.println("Power amp L is on");
        }
        else
        {
          Serial.println("Power amp L was already on");
        }
        
        if (getTemperature(A2) < 0)
        {
          setRelayOn(7);
          delay(200);
          if (getTemperature(A2) < 0)
            Serial.println("Check power to power amp R");
          else
            Serial.println("Power amp R is on");
        }
        else
        {
          Serial.println("Power amp R was already on");
        }
        
    }
}

// <TO DO: Should we have separate functions to call for the two relays?>
void RelayController::SetTriggerOff()
{
    mcp.digitalWrite(6,LOW);
    mcp.digitalWrite(7,LOW);
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

// Return masured temperature from 4.7K NTC connected to either A1 or A2
float RelayController::getTemperature(uint8_t pinNmbr)
{
  int sensorValue = 0;       // sensorPin default value
  float Vin = 5;             // Input voltage
  float Vout = 0;            // Vout default value
  float Rref = 4700;         // Reference resistor's value in ohms (you can give this value in kiloohms or megaohms - the resistance of the tested resistor will be given in the same units)
  float R = 0;               // Tested resistors default value
  float Temp = 0;
  
  sensorValue = analogRead(pinNmbr);    // Read Vout on analog input pin A0 (Arduino can sense from 0-1023, 1023 is 5V)
  Vout = (Vin * sensorValue) / 1023;    // Convert Vout to volts
  R = Rref * (1 / ((Vin / Vout) - 1));  // Formula to calculate tested resistor's value
  Temp = (-25.37 * log(R)) + 239.43;
  Serial.print("R: ");                  
  Serial.print(R);
  Serial.print(" = Temp: ");
  Serial.println(Temp);
  return(Temp);
}
/*
**
** Relay controller for MezmerizeB1Buffer
**
** Copyright (c) 2020 Carsten Grønning, Jan Abkjer Tofft
**
** Version 0.1
**
** PIN6 & PIN7 : Used for controlling amplifier triggers relays
** PIN0 - PIN5 : Used for controlling input relays
**                     
*/

#define NUM_OF_INPUTS 6

#include <Arduino.h>
#include <Adafruit_MCP23008.h>

class RelayController
{
public:
	RelayController();

	void begin();

	// Controlling input relays
	void setRelayOn(uint8_t Nmbr);
	void setRelayOff(uint8_t Nmbr);
	void setInput(uint8_t inputNmbr);
	void mute(boolean on);

	// Controlling triggers for amplifier.
	// <TO DO maybe we could have five options for the trigger circuit: "12V trigger latching", "12V trigger momentary", "momentary switch", "latching switch" and "custom" = my setup with the NTCs and LDRs ?>
	// Either use standard trigger 12 V or the alternate trigger, which uses NTCs and LDRs to sense if amplifier is turned on
	//void setAlternateTrigger(uint8_t inputRight = A2, uint8_t inputLeft = A3);
	void setStandardTrigger();
	void setTriggerOn();
	void SetTriggerOff();
	float getTemperature(uint8_t pinNmbr);

private:
	Adafruit_MCP23008 mcp;
	uint8_t selectedInput = 9;
	uint8_t sensorRight = A2;
	uint8_t sensorLeft = A3;
	boolean standardTrigger;
};

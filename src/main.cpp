/*
**
**    Controller for Mezmerize B1 Buffer using Muses7220 potentiometer 
**
**    Copyright (c) 2020 Carsten Grønning, Jan Abkjer Tofft
**
**    v1.0 12.2019
**
**
*/

#include "Arduino.h"
#include "RelayController.h"
#include "ClickEncoder.h"
#include "TimerOne.h"
#include "Muses72320.h"
#include "IRLremote.h"

// Setup Muses72320
// The address wired into the muses chip (usually 0).
static const byte MUSES_ADDRESS = 0;
static Muses72320 Muses(MUSES_ADDRESS);
static Muses72320::volume_t CurrentVolume = -20;


// Setup relay controller -----------------------------------------------------
RelayController rc;

void setupMuses72320() {
  
  // Initialize muses (SPI, pin modes)...
  Muses.begin();

  // Muses initially starts in a muted state, set a volume to enable sound.
  Muses.setVolume(CurrentVolume);

  // These are the default states and could be removed...
  Muses.setZeroCrossing(true);     // Enable/Disable zero crossing.
  Muses.setAttenuationLink(false); // Left channel controls both L/R gain channel.
  Muses.setGainLink(false);        // Left channel controls both L/R attenuation channel.

}

// Setup Rotary encoders ------------------------------------------------------
#define VERBOSECASE(label) case label: Serial.println(#label); break;

ClickEncoder *encoder1 = new ClickEncoder(5, 4, 3, 4);
ClickEncoder::Button button1;
int16_t e1last, e1value;

ClickEncoder *encoder2 = new ClickEncoder(8, 7, 6, 4);
ClickEncoder::Button button2;
int16_t e2last, e2value;

void timerIsr() {
  encoder1->service();
  encoder2->service();
}

void setupRotaryEncoders() {
    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr); 
}

// Setup IR -------------------------------------------------------------------
#define pinIR 2
CHashIR IRLremote;

void setupIR() {
  if (!IRLremote.begin(pinIR))
    Serial.println(F("You did not choose a valid pin."));
}



// Setup arduino nano ---------------------------------------------------------
void setup () {
   while (!Serial);
  Serial.begin(9600);
  Serial.println(F("Startup"));
  //setupMuses72320();
  setupRotaryEncoders();
  setupIR();
}


void loop () {

  e1value += encoder1->getValue();
  if(e1value > 0) e1value= 0; else if(e1value < -223) e1value = -223;

  if (e1value != e1last) {
    e1last = e1value;
    Serial.print("Encoder 1 - value : "); Serial.println(e1value);
    //Muses.setVolume(value);
    //delay(10);
  }

  e2value += encoder2->getValue();
  if (e2value != e2last) {
    e2last = e2value;
    Serial.print("Encoder 2 - value : "); Serial.println(e2value);
  }
  
  button1 = encoder1->getButton();
  switch (button1) {
    case ClickEncoder::Open : break;
    VERBOSECASE(ClickEncoder::Closed);
    VERBOSECASE(ClickEncoder::Pressed);
    VERBOSECASE(ClickEncoder::Held);
    VERBOSECASE(ClickEncoder::Clicked);
    VERBOSECASE(ClickEncoder::Released);
    VERBOSECASE(ClickEncoder::DoubleClicked);
  }
  
  button2 = encoder2->getButton();
  switch (button2) {
    case ClickEncoder::Open : break;
    VERBOSECASE(ClickEncoder::Closed);
    VERBOSECASE(ClickEncoder::Pressed);
    VERBOSECASE(ClickEncoder::Held);
    VERBOSECASE(ClickEncoder::Clicked);
    VERBOSECASE(ClickEncoder::Released);
    VERBOSECASE(ClickEncoder::DoubleClicked);
  }

  if (IRLremote.available())
  {
    // Get the new data from the remote
    auto data = IRLremote.read();

    // Print the protocol data
    Serial.print(F("Address: 0x"));
    Serial.println(data.address, HEX);
    Serial.print(F("Command: 0x"));
    Serial.println(data.command, HEX);
    Serial.println();
  }
     
}

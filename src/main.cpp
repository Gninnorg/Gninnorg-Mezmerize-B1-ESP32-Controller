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

#include <Arduino.h>
#include "RelayController.h"
#include "ClickEncoder.h"
#include "TimerOne.h"

RelayController rc;

ClickEncoder *encoder1 = new ClickEncoder(5, 4, 3, 4);
ClickEncoder::Button button;
int16_t last, value;


void timerIsr() {
  encoder1->service();
}

void setup () {
    Serial.begin(9600);
    Serial.println(F("Setup()"));
    rc.begin();
    rc.setTriggerOn();
    rc.setInput(1);
    rc.setInputName(1, F("DAC"));
    Serial.println(rc.getInputName(rc.getInput()));
    delay(5000);
    rc.setInput(2);
    rc.setInputName(2, F("DAC2"));
    Serial.println(rc.getInputName(rc.getInput()));
    delay(1000);
    rc.setInput(3);
    delay(1000);
    rc.setInput(4);
    delay(1000);
    rc.setInput(5);
    delay(1000);
    rc.setInput(6);
    Serial.println(rc.getInputName(rc.getInput()));
    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr); 
}


void loop () {

  value += encoder1->getValue();
    
  if (value != last) {
    last = value;
    Serial.print("Encoder Value: ");
    Serial.println(value);
  }
  
  button = encoder1->getButton();
  if (button != ClickEncoder::Open) {
    Serial.print("Button: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (button) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
          encoder1->setAccelerationEnabled(!encoder1->getAccelerationEnabled());
          Serial.print("  Acceleration is ");
          Serial.println((encoder1->getAccelerationEnabled()) ? "enabled" : "disabled");
        break;
    }
  }    
}

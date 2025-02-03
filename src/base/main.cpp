#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include "MyIoT.h"


void setup() {
  // put your setup code here, to run once:

    Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
    Serial.println("Hello World");
    delay(1000);
}


#include "MyIoT.h"

// Constructor: Initialize the pin
MyIoT::MyIoT(int pin) {
  _pin = pin;
}

// Initialize the library
void MyIoT::begin() {
  pinMode(_pin, OUTPUT); // Set the pin as an output
}

// Turn on the LED
void MyIoT::turnOn() {
  digitalWrite(_pin, HIGH);
}

// Turn off the LED
void MyIoT::turnOff() {
  digitalWrite(_pin, LOW);
}
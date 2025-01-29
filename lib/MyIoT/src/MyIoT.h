#ifndef MYIOT_H
#define MYIOT_H

#include <Arduino.h> // Include Arduino core functions

class MyIoT {
  public:
    MyIoT(int pin); // Constructor
    void begin();   // Initialize the library
    void turnOn();  // Turn on the LED
    void turnOff(); // Turn off the LED

  private:
    int _pin;       // Private variable to store the pin number
};

#endif
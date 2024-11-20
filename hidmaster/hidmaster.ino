// Wire Master Reader
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Reads data from an I2C/TWI slave device
// Refer to the "Wire Slave Sender" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <Joystick.h>
#include <stdio.h>
extern "C" {
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "twi.h"
}

#define JOYSTICK_COUNT 3
#define PRINT 0

Joystick_ joysticks[JOYSTICK_COUNT] = {
  Joystick_(0x03, JOYSTICK_TYPE_JOYSTICK, 7, 1, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x04, JOYSTICK_TYPE_JOYSTICK, 7, 1, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x05, JOYSTICK_TYPE_JOYSTICK, 7, 1, true, true, false, false, false, false, false, false, false, false, false)
};

void flash_led() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1);
  digitalWrite(LED_BUILTIN, 0);
  delay(1);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  twi_init();
  twi_setFrequency(400 * 1000);
  Serial.begin(9600);           // start serial for output
  //while (!Serial) flash_led();  // Leonardo: wait for Serial Monitor
  Serial.println("\nSetup");
  for (uint8_t i = 0; i < JOYSTICK_COUNT; i++) {
    joysticks[i].setXAxisRange(1, 255);
    joysticks[i].setYAxisRange(1, 255);
    joysticks[i].begin();
  }
}

struct packet {
  uint8_t button_state;
  uint8_t joystickX_val;
  uint8_t joystickY_val;
};

void updateHID(uint8_t id, struct packet *pct) {
  joysticks[id].setXAxis(pct->joystickX_val);
  joysticks[id].setYAxis(pct->joystickY_val);
  uint8_t mask = 1;
  for (uint8_t i = 0; i < 8; i++) {
    joysticks[id].setButton(i, pct->button_state & mask);
    mask = mask << 1;
    //Serial.println(mask);
  }
  flash_led(); 
}

void loop() {
  for (uint8_t addr = 0x10; addr <= 0x12; addr++) {
    struct packet thispacket = { 0, 0, 0 };
    char buf[64] = { 0 };
    uint8_t read = twi_readFrom(addr, (uint8_t *)&thispacket, sizeof(thispacket), true);
    //Serial.println(read);
    sprintf(buf, "%x %x %x %x", addr, thispacket.button_state, thispacket.joystickX_val, thispacket.joystickY_val);
    if (PRINT &&thispacket.joystickX_val != 0) Serial.println(buf);

    updateHID(addr - 0x10, &thispacket);
    //delay(100);
  }
}

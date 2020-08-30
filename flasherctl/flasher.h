#ifndef __FLASHER_H__
#define __FLASHER_H__

#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>

// Subsystem ID for the flashers
// The iostack is "0"
// The Dynamixel servos are "1"
#define SYS_FLASHER 2

// Error codes
#define FLASHER_EBVALUE       1
#define FLASHER_ESERIALNO     2
#define FLASHER_ETIMEDOUT     128
#define FLASHER_ERXCHECKSUM   256
#define FLASHER_EMISMATCH     512
#define FLASHER_ERDONLY       1024
#define FLASHER_ERXFSM        32768

// WIZnet W5500 Ethernet (SPI_2 : SERCOM4)
// CS for the W5500 is "SS"

// I2C/SPI Header (SPI_0 : SERCOM0)
#define SCOM0_0 0 // PA04
#define SCOM0_1 1 // PA05
#define SCOM0_2 2 // PA06
#define SCOM0_3 3 // PA07

// ADT7310 Temperature Sensor on the LED Board (SPI_1 : SERCOM1)
#define ADT7310_CS 7 // PB08

// DS1023-25
#define DS1023_LE 9   // PA22
#define DS1023_CLK 16 // PA21
#define DS1023_D 15   // PA20

// LED Current
#define LED_A0 21 // PA14
#define LED_A1 20 // PA13
#define LED_A2 6  // PA12
#define LED_A3 22 // PA15

#define SPI_SPEED 1000000 // Default to 1MHz SPI transfers

uint16_t flasher_LED_BUILTIN(uint8_t on_off);
uint16_t flasher_START_TEMPERATURE();
float flasher_READ_TEMPERATURE(uint16_t *error);
uint16_t flasher_READ_SERIAL_NO(uint8_t *serial_no);
uint16_t flasher_SET_LED_CURRENT(uint8_t current);
uint16_t flasher_SET_PULSE_WIDTH(uint8_t width);
uint16_t flasher_TEST_PULSE(uint8_t on_off);

void start_temperature(SPIClass this_spi, int this_cs);
float read_temperature(SPIClass this_spi, int this_cs);

#endif

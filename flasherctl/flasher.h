#ifndef __FLASHER_H__
#define __FLASHER_H__

#include <Arduino.h>

#include <SPI.h>

// Subsystem ID for the flashers
// The iostack is "0"
// The Dynamixel servos are "1"
#define SYS_FLASHER 2

// Error codes
#define FLASHER_EBVALUE       1
#define FLASHER_EFLASHERID    2
#define FLASHER_ETIMEDOUT     128
#define FLASHER_ERXCHECKSUM   256
#define FLASHER_EMISMATCH     512
#define FLASHER_ERDONLY       1024
#define FLASHER_ERXFSM        32768

// CS for the W5500 is "SS"
// Define SPI CS pins for the four timing boards
#define CS1_1 8 // PA22
#define CS1_2 7 // PA16
#define CS1_3 6 // PA12
#define CS1_4 5 // PA08
#define CS2_1 0 // PA04
#define CS2_2 1 // PA05
#define CS2_3 2 // PA06
#define CS2_4 3 // PA07

#define SPI_SPEED 1000000 // Default to 1MHz SPI transfers for the timing boards

// Definitions for the MCP23S17 registers
#define    IODIRA    (0x00)      // MCP23x17 I/O Direction Register
#define    IODIRB    (0x01)      // 1 = Input (default), 0 = Output
#define    IPOLA     (0x02)      // MCP23x17 Input Polarity Register
#define    IPOLB     (0x03)      // 0 = Normal (default)(low reads as 0), 1 = Inverted (low reads as 1)
#define    GPINTENA  (0x04)      // MCP23x17 Interrupt on Change Pin Assignements
#define    GPINTENB  (0x05)      // 0 = No Interrupt on Change (default), 1 = Interrupt on Change
#define    DEFVALA   (0x06)      // MCP23x17 Default Compare Register for Interrupt on Change
#define    DEFVALB   (0x07)      // Opposite of what is here will trigger an interrupt (default = 0)
#define    INTCONA   (0x08)      // MCP23x17 Interrupt on Change Control Register
#define    INTCONB   (0x09)      // 1 = pin is compared to DEFVAL, 0 = pin is compared to previous state (default)
#define    IOCON     (0x0A)      // MCP23x17 Configuration Register
//                   (0x0B)      //     Also Configuration Register
#define    GPPUA     (0x0C)      // MCP23x17 Weak Pull-Up Resistor Register
#define    GPPUB     (0x0D)      // INPUT ONLY: 0 = No Internal 100k Pull-Up (default) 1 = Internal 100k Pull-Up 
#define    INTFA     (0x0E)      // MCP23x17 Interrupt Flag Register
#define    INTFB     (0x0F)      // READ ONLY: 1 = This Pin Triggered the Interrupt
#define    INTCAPA   (0x10)      // MCP23x17 Interrupt Captured Value for Port Register
#define    INTCAPB   (0x11)      // READ ONLY: State of the Pin at the Time the Interrupt Occurred
#define    GPIOA     (0x12)      // MCP23x17 GPIO Port Register
#define    GPIOB     (0x13)      // Value on the Port - Writing Sets Bits in the Output Latch
#define    OLATA     (0x14)      // MCP23x17 Output Latch Register
#define    OLATB     (0x15)      // 1 = Latch High, 0 = Latch Low (default) Reading Returns Latch State, Not Port Value!


uint16_t flasher_LED_BUILTIN(uint8_t on_off);
uint16_t flasher_START_TEMPERATURE(uint8_t board);
float flasher_READ_TEMPERATURE(uint8_t board, uint16_t *error);
uint16_t flasher_INIT_FLASHER(uint8_t board);
uint16_t flasher_SET_GPO_PIN(uint8_t board, uint8_t pin, uint8_t on_off);
uint16_t flasher_READ_SERIAL_NO(uint8_t board, uint8_t *serial_no);
uint16_t flasher_SET_LEDS(uint8_t board, uint8_t hi_leds, uint8_t lo_leds);
uint16_t flasher_SET_PULSE_WIDTH(uint8_t board, uint8_t width);
uint16_t flasher_TEST_PULSE(uint8_t on_off);
uint16_t flasher_READ_TRIG(uint8_t board, uint8_t *trig_ptr);

void start_temperature(SPIClass this_spi, int this_cs);
float read_temperature(SPIClass this_spi, int this_cs);
void init_flasher(SPIClass this_spi, int this_cs);
void set_gpo_pin(SPIClass this_spi, int this_cs, uint8_t pin, uint8_t on_off);
void read_serial_no(SPIClass this_spi, int this_cs, uint8_t *serial_no);
void set_leds(SPIClass this_spi, int this_cs, uint8_t hi_leds, uint8_t lo_leds);
void set_pulse_width(SPIClass this_spi, int this_cs, uint8_t width);
void read_trig(SPIClass this_spi, int this_cs, uint8_t *val);

void write_i2c_byte(SPIClass this_spi, int this_cs, uint8_t the_byte, uint8_t iopins);
uint8_t get_ack(SPIClass this_spi, int this_cs, uint8_t iopins);
uint8_t read_i2c_byte(SPIClass this_spi, int this_cs, uint8_t iopins);
void set_ack(SPIClass this_spi, int this_cs, uint8_t iopins);
void set_nack(SPIClass this_spi, int this_cs, uint8_t iopins);

#endif

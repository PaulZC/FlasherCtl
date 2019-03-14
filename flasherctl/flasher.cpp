#include <Arduino.h>

#include "flasher.h"

extern SPIClass SPI_1;
extern SPIClass SPI_2;
extern SPIClass SPI_3;
extern SPIClass SPI_4;

/* Sets LED_BUILTIN high or low */
uint16_t flasher_LED_BUILTIN(uint8_t on_off)
{
  uint16_t error = 0;
  
  if ((on_off != 0) && (on_off != 1)) {
    error = FLASHER_EBVALUE;
  } else {
    digitalWrite(LED_BUILTIN, on_off);
  }

  return error;
}

/* Sets TEST_PULSE / PIN_LED2 high or low. If on_off is 2, PIN_LED2 is pulsed high then low*/
uint16_t flasher_TEST_PULSE(uint8_t on_off)
{
  uint16_t error = 0;
  
  if (on_off == 0)
  {
    digitalWrite(PIN_LED2, 0);
  }
  else if (on_off == 1)
  {
    digitalWrite(PIN_LED2, 1);
  }
  else if (on_off == 2)
  {
    digitalWrite(PIN_LED2, 1);
    digitalWrite(PIN_LED2, 1); // NOP
    digitalWrite(PIN_LED2, 1); // NOP
    digitalWrite(PIN_LED2, 1); // NOP
    digitalWrite(PIN_LED2, 1); // NOP
    digitalWrite(PIN_LED2, 0);
  }
  else
  {
    error = FLASHER_EBVALUE;
  }

  return error;
}

/* Starts the ADT7310 temperature measurement (16-bit mode) on the given board */
uint16_t flasher_START_TEMPERATURE(uint8_t board)
{
  uint16_t error = 0;

  if (board == 0) {
    start_temperature(SPI_1, CS2_1);
  }
  else if (board == 1) {
    start_temperature(SPI_2, CS2_2);
  }
  else if (board == 2) {
    start_temperature(SPI_3, CS2_3);
  }
  else if (board == 3) {
    start_temperature(SPI_4, CS2_4);
  }
  else {
    error = FLASHER_EFLASHERID;
  }

  return error;
}

/* Reads the ADT7310 temperature (16-bit mode) from the given board */
float flasher_READ_TEMPERATURE(uint8_t board, uint16_t *error)
{
  float temperature;

  if (board == 0) {
    temperature = read_temperature(SPI_1, CS2_1);
  }
  else if (board == 1) {
    temperature = read_temperature(SPI_2, CS2_2);
  }
  else if (board == 2) {
    temperature = read_temperature(SPI_3, CS2_3);
  }
  else if (board == 3) {
    temperature = read_temperature(SPI_4, CS2_4);
  }
  else {
    *error = FLASHER_EFLASHERID;
  }

  return temperature;
}

void start_temperature(SPIClass this_spi, int this_cs)
{
  // Start a new 16-bit single shot conversion to avoid blocking code (otherwise we need to wait 240ms for conversion to complete)
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3)); // ADT7310 needs SPI Mode 3
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x08); // Send command byte: Write, Register 1 (configuration register)
  this_spi.transfer(0xA0); // Send data byte: 16-bit mode (1), one shot (01), interrupt mode (0), INT active low (0), CT active low (0), 1 fault (00)
  digitalWrite(this_cs,HIGH); // Pull CS high
  this_spi.endTransaction();
}

float read_temperature(SPIClass this_spi, int this_cs)
{
  uint8_t hi_byte;
  uint8_t lo_byte;
  int16_t temperature_bits; // Two's complement
  float temperature;

  // Read temperature from this SPI using this CS
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3)); // ADT7310 needs SPI Mode 3
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x50); // Send command byte: Read, Register 2 (temperature value register (16-bit))
  hi_byte = this_spi.transfer(0xff); // Read MSB
  lo_byte = this_spi.transfer(0xff); // Read LSB
  digitalWrite(this_cs,HIGH); // Pull CS high
  this_spi.endTransaction();

  // Convert the temperature to float
  temperature_bits = (((uint16_t)hi_byte) << 8) | ((uint16_t)lo_byte);
  temperature = (float)temperature_bits;
  temperature = temperature / 128.0;
  
  return temperature;
}

/* Initialises the MCP23S17 I/O pins on the given board */
uint16_t flasher_INIT_FLASHER(uint8_t board)
{
  uint16_t error = 0;

  if (board == 0) {
    init_flasher(SPI_1, CS1_1);
  }
  else if (board == 1) {
    init_flasher(SPI_2, CS1_2);
  }
  else if (board == 2) {
    init_flasher(SPI_3, CS1_3);
  }
  else if (board == 3) {
    init_flasher(SPI_4, CS1_4);
  }
  else {
    error = FLASHER_EFLASHERID;
  }

  return error;
}

void init_flasher(SPIClass this_spi, int this_cs)
{
  // Initialise the MCP23S17 I/O pins
  // MCP23S17 can run in SPI Mode 0 or 3
  // Let's use Mode 3 to keep it the same as the ADT7310
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3));
  // The datasheet says we need to toggle CS once to enable Mode 3
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP

  
  // First write to both IOCON registers and enable hardware addressing via HAEN
  // IOCON address shoudl be 0x0A (or 0x0B) at power-up since the BANK bit should be clear
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(IOCON); // Send IOCON register address
  // 7:BANK:0 6:MIRROR:0 5:SEQOP:1 4:DISSLW:1 3:HAEN:1 2:ODR:0 1:INTPOL:0 0:N/I:0
  this_spi.transfer(0x38); // Disable sequential operation, disable slew rate control, enable address pins
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP

  // Do it again just in case!
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(IOCON); // Send IOCON register address
  this_spi.transfer(0x38); // Disable sequential operation, disable slew rate control, enable address pins
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP

  // Once more for luck!
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(IOCON); // Send IOCON register address
  this_spi.transfer(0x38); // Disable sequential operation, disable slew rate control, enable address pins
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP

  // The two MCP23S17's can now be addressed independently
  // For the MCP23S17 with address 0 (timing pulse width control and General Purpose Output (GPO) pins)
  // Set all Port A bits to outputs and set them low (sets DS1023 pulse width to the minimum)
  // Set Port B bit 0 to output and set it low (DS1023 Latch Enable)
  // Set Port B bit 1 to input without pull-up (trigger pulse)
  // Set Port B bits 2 & 3 to output and set them high (disable GPO 0 and 1)
  // Set Port B bits 4-7 to inputs with pull-up
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(IODIRA); // Send IODIRA register address
  this_spi.transfer(0x00); // Set all eight bits to outputs
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(GPIOA); // Send GPIOA register address
  this_spi.transfer(0x00); // Set all eight bits low
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(IODIRB); // Send IODIRB register address
  this_spi.transfer(0xF2); // Set bits 7-4 & 1 to inputs
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(0x0C); // Set bit 0 low, bits 2 & 3 high
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(GPPUB); // Send GPPUB register address
  this_spi.transfer(0xF0); // Enable pull-ups on bits 7-4
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // For the MCP23S17 with address 1 (LED control and serial number)
  // Set all Port A bits to outputs and set them high (disable the LEDs)
  // Set Port B bits 0 and 1 to outputs
  // Set Port B bits 0 and 1 high (disable LEDs 9 & 10)
  // Set Port B bits 4 and 5 to inputs (serial number SCL and SDA)
  // Set Port B bits 6 and 7 to outputs and set them low (serial number SCL and SDA to idle)
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(IODIRA); // Send IODIRA register address
  this_spi.transfer(0x00); // Set all eight bits to outputs
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOA); // Send GPIOA register address
  this_spi.transfer(0xFF); // Set all eight bits high
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(0x03); // Set bits 0 and 1 high, set bits 6 and 7 low
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(IODIRB); // Send IODIRB register address
  this_spi.transfer(0x30); // Set bits 4 & 5 to inputs
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  this_spi.endTransaction();
}

uint16_t flasher_SET_GPO_PIN(uint8_t board, uint8_t pin, uint8_t on_off)
// Sets the selected GPO pin on the selected board on or off
{
  uint16_t error = 0;

  if (board == 0) {
    set_gpo_pin(SPI_1, CS1_1, pin, on_off);
  }
  else if (board == 1) {
    set_gpo_pin(SPI_2, CS1_2, pin, on_off);
  }
  else if (board == 2) {
    set_gpo_pin(SPI_3, CS1_3, pin, on_off);
  }
  else if (board == 3) {
    set_gpo_pin(SPI_4, CS1_4, pin, on_off);
  }
  else {
    error = FLASHER_EFLASHERID;
  }

  return error;
}

void set_gpo_pin(SPIClass this_spi, int this_cs, uint8_t pin, uint8_t on_off)
{
  // Set the selected GPO pin on or off
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3));
  // The datasheet says we need to toggle CS once to enable Mode 3
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  
  // For the MCP23S17 with address 0 (timing pulse width control and General Purpose Output (GPO) pins)
  // Read the GPIOB register and set or clear bit 2 or 3 as appropriate
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x41); // Send control byte: slave address 0, read
  this_spi.transfer(GPIOB); // Send GPIOB register address
  uint8_t iopins = this_spi.transfer(0xFF); // Read the current pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  if (pin == 0) {
    // Set or clear GPO0 (bit 2)
    if (on_off == 0) {
      // Set GPO0 (logic high disables the LED)
      iopins |= 0x04; // Set bit 2
    }
    else {
      // Clear GPO0 (logic low enables the LED)
      iopins &= 0xFB; // Clear bit 2
    }
  }
  else {
    // Set or clear GPO1 (bit 3)
    if (on_off == 0) {
      // Set GPO1 (logic high disables the LED)
      iopins |= 0x08; // Set bit 3
    }
    else {
      // Clear GPO1 (logic low enables the LED)
      iopins &= 0xF7; // Clear bit 3
    }    
  }
  
  // Write the modified pin settings back into GPIOB
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Write the modified pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  this_spi.endTransaction();
}

uint16_t flasher_SET_LEDS(uint8_t board, uint8_t hi_leds, uint8_t lo_leds)
// Configures the LEDs on the selected board
{
  uint16_t error = 0;

  if (board == 0) {
    set_leds(SPI_1, CS1_1, hi_leds, lo_leds);
  }
  else if (board == 1) {
    set_leds(SPI_2, CS1_2, hi_leds, lo_leds);
  }
  else if (board == 2) {
    set_leds(SPI_3, CS1_3, hi_leds, lo_leds);
  }
  else if (board == 3) {
    set_leds(SPI_4, CS1_4, hi_leds, lo_leds);
  }
  else {
    error = FLASHER_EFLASHERID;
  }

  return error;
}

void set_leds(SPIClass this_spi, int this_cs, uint8_t hi_leds, uint8_t lo_leds)
{
  // Configure the ten LED pins
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3));
  // The datasheet says we need to toggle CS once to enable Mode 3
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  
  // For the MCP23S17 with address 1 (LED control and serial number)
  // Invert the 8 LS bits of leds and write into GPIOA
  uint8_t LSB = lo_leds; // Get the 8 LS bits
  LSB = LSB ^ 0xFF; // Invert them using ^ (Ex-OR)
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOA); // Send GPIOA register address
  this_spi.transfer(LSB); // Update LEDs 1-8
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Read the current pin settings from GPIOB
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x43); // Send control byte: slave address 1, read
  this_spi.transfer(GPIOB); // Send GPIOB register address
  uint8_t portb = this_spi.transfer(0xFF); // Read pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  LSB = hi_leds; // Get the 2 MS bits
  LSB &= 0x03; // Make sure the other six bits are zero
  LSB = LSB ^ 0x03; // Invert only the two LED bits
  portb &= 0xFC; // Clear the two LED bits in portb
  portb |= LSB; // OR the two inverted LEDs into portb

  // Update the pin settings in GPIOB
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(portb); // Update pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  this_spi.endTransaction();
}

uint16_t flasher_SET_PULSE_WIDTH(uint8_t board, uint8_t width)
// Configures the pulse width (DS1023 delay) on the selected board
{
  uint16_t error = 0;

  if (board == 0) {
    set_pulse_width(SPI_1, CS1_1, width);
  }
  else if (board == 1) {
    set_pulse_width(SPI_2, CS1_2, width);
  }
  else if (board == 2) {
    set_pulse_width(SPI_3, CS1_3, width);
  }
  else if (board == 3) {
    set_pulse_width(SPI_4, CS1_4, width);
  }
  else {
    error = FLASHER_EFLASHERID;
  }

  return error;
}

void set_pulse_width(SPIClass this_spi, int this_cs, uint8_t width)
{
  // Configure the eight delay pins
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3));
  // The datasheet says we need to toggle CS once to enable Mode 3
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  
  // For the MCP23S17 with address 0 (timing pulse width control and General Purpose Output (GPO) pins)
  // Write width into GPIOA
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(GPIOA); // Send GPIOA register address
  this_spi.transfer(width); // Update pulse width
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Pull LE high:
  // Read the current pin settings from GPIOB
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x41); // Send control byte: slave address 0, read
  this_spi.transfer(GPIOB); // Send GPIOB register address
  uint8_t portb = this_spi.transfer(0xFF); // Read pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  portb |= 0x01; // Make sure LE (bit 0) is high

  // Update the pin settings in GPIOB
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(portb); // Update pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Pull LE low:
  portb &= 0xFE; // Make sure LE (bit 0) is low

  // Update the pin settings in GPIOB
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x40); // Send control byte: slave address 0, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(portb); // Update pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  this_spi.endTransaction();
}

uint16_t flasher_READ_TRIG(uint8_t board, uint8_t *val)
// Reads the state of the TRIG signal on the selected board
{
  uint16_t error = 0;

  if (board == 0) {
    read_trig(SPI_1, CS1_1, val);
  }
  else if (board == 1) {
    read_trig(SPI_2, CS1_2, val);
  }
  else if (board == 2) {
    read_trig(SPI_3, CS1_3, val);
  }
  else if (board == 3) {
    read_trig(SPI_4, CS1_4, val);
  }
  else {
    error = FLASHER_EFLASHERID;
  }

  return error;
}

void read_trig(SPIClass this_spi, int this_cs, uint8_t *val)
{
  // Read PortB bit 1 from SPI expander 0
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3));
  // The datasheet says we need to toggle CS once to enable Mode 3
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  
  // For the MCP23S17 with address 0 (timing pulse width control and General Purpose Output (GPO) pins)
  // Read the current pin settings from GPIOB
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x41); // Send control byte: slave address 0, read
  this_spi.transfer(GPIOB); // Send GPIOB register address
  uint8_t portb = this_spi.transfer(0xFF); // Read pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  *val = (portb & 0x02) >> 1; // Mask off bit 1 and shift right into LS bit

  this_spi.endTransaction();
}

uint16_t flasher_READ_SERIAL_NO(uint8_t board, uint8_t *serial_no)
// Read the DS28CM00 serial number from the selected board via the MCP23S17 pins
{
  uint16_t error = 0;

  if (board == 0) {
    read_serial_no(SPI_1, CS1_1, serial_no);
  }
  else if (board == 1) {
    read_serial_no(SPI_2, CS1_2, serial_no);
  }
  else if (board == 2) {
    read_serial_no(SPI_3, CS1_3, serial_no);
  }
  else if (board == 3) {
    read_serial_no(SPI_4, CS1_4, serial_no);
  }
  else {
    error = FLASHER_EFLASHERID;
  }

  return error;  
}

void read_serial_no(SPIClass this_spi, int this_cs, uint8_t *serial_no)
{
  // The DS28CM00 SCL and SDA pins are connected to the MCP23S17 with Address 1
  // Port B bit 4 is an input for SCL
  // Port B bit 5 is an input for SDA
  // Port B bit 6 is an output for SCL, setting bit 6 high will pull SCL low
  // Port B bit 7 is an output for SDA, setting bit 7 high will pull SDA low
  // We need to bit-bang these bits to mimic an I2C transfer

  // Start the SPI transaction
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3));
  // The datasheet says we need to toggle CS once to enable Mode 3
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,LOW); // Pull CS low
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,LOW); // NOP
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  digitalWrite(this_cs,HIGH); // NOP
  
  // Begin with the "START Condition": set SDA low while SCL remains high
  // First read GPIOB
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x43); // Send control byte: slave address 1, read
  this_spi.transfer(GPIOB); // Send GPIOB register address
  uint8_t iopins = this_spi.transfer(0xFF); // Get the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  // Set Port B bit 7 high to pull SDA low
  iopins |= 0x80; // Set bit 7 (SDA)
  // Make sure Port B bit 6 is low to pull SCL high
  iopins &= 0xBF; // Clear bit 6 (SCL)
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  // Now pull SCL low
  // Set Port B bit 6 to pull SCL low
  iopins |= 0x40; // Set bit 6 (SCL)
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Write slave address with read/!write bit clear
  write_i2c_byte(this_spi, this_cs, 0xA0, iopins);
  get_ack(this_spi, this_cs, iopins); // Get the ack bit and ignore it

  // Send zero address
  write_i2c_byte(this_spi, this_cs, 0x00, iopins);
  get_ack(this_spi, this_cs, iopins); // Get the ack bit and ignore it

  // Repeat the START condition: set SDA high, then set SCL high, then set SDA low, then set SCL low
  // Set SDA high
  // Clear bit 7 to pull SDA high
  iopins &= 0x7F; // Clear bit 7 to pull SDA high
  // Set bit 6 to pull SCL low
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  // Set SCL high
  // Clear bit 6 to pull SCL high
  iopins &= 0xBF; // Clear bit 6 to pull SCL high
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  // Set SDA low
  // Set bit 7 to pull SDA low
  iopins |= 0x80; // Set bit 7 to pull SDA low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  // Set SCL low
  // Set bit 6 to pull SCL low
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Write slave address with read/!write bit set
  write_i2c_byte(this_spi, this_cs, 0xA1, iopins);
  get_ack(this_spi, this_cs, iopins); // Get the ack bit and ignore it

  // Read and discard first byte (device family code = 0x70)
  read_i2c_byte(this_spi, this_cs, iopins);
  set_ack(this_spi, this_cs, iopins);

  // Now read the six serial number bytes
  serial_no[5] = read_i2c_byte(this_spi, this_cs, iopins);
  set_ack(this_spi, this_cs, iopins);
  serial_no[4] = read_i2c_byte(this_spi, this_cs, iopins);
  set_ack(this_spi, this_cs, iopins);
  serial_no[3] = read_i2c_byte(this_spi, this_cs, iopins);
  set_ack(this_spi, this_cs, iopins);
  serial_no[2] = read_i2c_byte(this_spi, this_cs, iopins);
  set_ack(this_spi, this_cs, iopins);
  serial_no[1] = read_i2c_byte(this_spi, this_cs, iopins);
  set_ack(this_spi, this_cs, iopins);
  serial_no[0] = read_i2c_byte(this_spi, this_cs, iopins);
  set_nack(this_spi, this_cs, iopins); // nack leaves SDA high

  // Finish with the "STOP Condition": set SDA is low, then set SCL high, then set SDA high
  // Set SDA low
  // Set bit 7 to pull SDA low
  iopins |= 0x80; // Set bit 7 to pull SDA low
  // Set bit 6 to pull SCL low
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  // Set SCL high
  // Clear bit 6 to pull SCL high
  iopins &= 0xBF; // Clear bit 6 to pull SCL high
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  // Set SDA high
  // Clear bit 7 to pull SDA high
  iopins &= 0x7F; // Clear bit 7 to pull SDA high
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  this_spi.endTransaction();
}

void write_i2c_byte(SPIClass this_spi, int this_cs, uint8_t the_byte, uint8_t iopins)
// Clock eight bits on SDA, MSB first
{
  for (uint8_t x = B10000000; x != 0; x = x >> 1)
  {
    uint8_t the_bit = the_byte & x;
    if (the_bit == 0x00) // If the bit is clear
    {
      iopins |= 0x80; // Set bit 7 to pull SDA low
    }
    else
    {
      iopins &= 0x7F; // Clear bit 7 to pull SDA high
    }
    iopins |= 0x40; // Make sure bit 6 is high to pull SCL low
    digitalWrite(this_cs,LOW); // Pull CS low
    this_spi.transfer(0x42); // Send control byte: slave address 1, write
    this_spi.transfer(GPIOB); // Send GPIOB register address
    this_spi.transfer(iopins); // Update the pin settings
    digitalWrite(this_cs,HIGH); // Pull CS high
    digitalWrite(this_cs,HIGH); // NOP
    
    iopins &= 0xBF; // Clear bit 6 to pull SCL high
    digitalWrite(this_cs,LOW); // Pull CS low
    this_spi.transfer(0x42); // Send control byte: slave address 1, write
    this_spi.transfer(GPIOB); // Send GPIOB register address
    this_spi.transfer(iopins); // Update the pin settings
    digitalWrite(this_cs,HIGH); // Pull CS high
    digitalWrite(this_cs,HIGH); // NOP
    
    iopins |= 0x40; // Set bit 6 to pull SCL low
    digitalWrite(this_cs,LOW); // Pull CS low
    this_spi.transfer(0x42); // Send control byte: slave address 1, write
    this_spi.transfer(GPIOB); // Send GPIOB register address
    this_spi.transfer(iopins); // Update the pin settings
    digitalWrite(this_cs,HIGH); // Pull CS high
    digitalWrite(this_cs,HIGH); // NOP
  }
}

uint8_t get_ack(SPIClass this_spi, int this_cs, uint8_t iopins)
{
  // Make sure SDA is high so ACK bit can be read
  // Clear bit 7 to pull SDA high
  iopins &= 0x7F; // Clear bit 7 to pull SDA high
  // Make sure bit 6 is high to pull SCL low
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Now raise the clock line
  iopins &= 0xBF; // Clear bit 6 to pull SCL high
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Now read the SDA pin to sample the ack bit
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x43); // Send control byte: slave address 1, read
  this_spi.transfer(GPIOB); // Send GPIOB register address
  uint8_t ack_bit = this_spi.transfer(0xFF); // Read the pins, ack bit will be in bit 5
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Now drop the clock line
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Finally, return the ack bit (bit 5)
  ack_bit &= 0x20; // Clear the other bits
  ack_bit = ack_bit >> 5; // Shift the ack bit into the LSB
  return ack_bit;
}

uint8_t read_i2c_byte(SPIClass this_spi, int this_cs, uint8_t iopins)
{
  // Make sure SDA is high so bits can be read
  // Clear bit 7 to pull SDA high
  iopins &= 0x7F; // Clear bit 7 to pull SDA high
  // Make sure bit 6 is high to pull SCL low
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
  
  uint8_t rx_byte = 0x00;

  // Read the eight bits one at a time
  for (uint8_t x = B10000000; x != 0; x = x >> 1)
  {
    // Now raise the clock line
    iopins &= 0xBF; // Clear bit 6 to pull SCL high
    digitalWrite(this_cs,LOW); // Pull CS low
    this_spi.transfer(0x42); // Send control byte: slave address 1, write
    this_spi.transfer(GPIOB); // Send GPIOB register address
    this_spi.transfer(iopins); // Update the pin settings
    digitalWrite(this_cs,HIGH); // Pull CS high
    digitalWrite(this_cs,HIGH); // NOP

    // Now read the SDA pin to sample the data bit
    digitalWrite(this_cs,LOW); // Pull CS low
    this_spi.transfer(0x43); // Send control byte: slave address 1, read
    this_spi.transfer(GPIOB); // Send GPIOB register address
    uint8_t data_byte = this_spi.transfer(0xFF); // Read the pins
    digitalWrite(this_cs,HIGH); // Pull CS high
    digitalWrite(this_cs,HIGH); // NOP

    // If bit 5 is a '1', OR rx_byte with x
    uint8_t data_bit = data_byte & 0x20;
    if (data_bit == 0x20)
    {
      rx_byte = rx_byte | x;
    }

    // Now drop the clock line
    iopins |= 0x40; // Set bit 6 to pull SCL low
    digitalWrite(this_cs,LOW); // Pull CS low
    this_spi.transfer(0x42); // Send control byte: slave address 1, write
    this_spi.transfer(GPIOB); // Send GPIOB register address
    this_spi.transfer(iopins); // Update the pin settings
    digitalWrite(this_cs,HIGH); // Pull CS high
    digitalWrite(this_cs,HIGH); // NOP
  }

  return rx_byte;
}

void set_ack(SPIClass this_spi, int this_cs, uint8_t iopins)
{
  // Set SDA low to ack the byte
  // Set bit 7 to pull SDA low
  iopins |= 0x80; // Set bit 7 to pull SDA low
  // Make sure bit 6 is high to pull SCL low
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Now raise the clock line
  iopins &= 0xBF; // Clear bit 6 to pull SCL high
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Now drop the clock line
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
}

void set_nack(SPIClass this_spi, int this_cs, uint8_t iopins)
{
  // Set SDA high to nack the byte
  // Clear bit 7 to pull SDA high
  iopins &= 0x7F; // Clear bit 7 to pull SDA high
  // Make sure bit 6 is high to pull SCL low
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Now raise the clock line
  iopins &= 0xBF; // Clear bit 6 to pull SCL high
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP

  // Now drop the clock line
  iopins |= 0x40; // Set bit 6 to pull SCL low
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x42); // Send control byte: slave address 1, write
  this_spi.transfer(GPIOB); // Send GPIOB register address
  this_spi.transfer(iopins); // Update the pin settings
  digitalWrite(this_cs,HIGH); // Pull CS high
  digitalWrite(this_cs,HIGH); // NOP
}




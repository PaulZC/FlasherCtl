#include <Arduino.h>

#include "flasher.h"

extern SPIClass SPI_1;
extern SPIClass SPI_2;

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
    digitalWrite(PIN_LED2, LOW);
  }
  else if (on_off == 1)
  {
    digitalWrite(PIN_LED2, HIGH);
  }
  else if (on_off == 2)
  {
    digitalWrite(PIN_LED2, HIGH);
    digitalWrite(PIN_LED2, HIGH); // NOP
    digitalWrite(PIN_LED2, HIGH); // NOP
    digitalWrite(PIN_LED2, HIGH); // NOP
    digitalWrite(PIN_LED2, HIGH); // NOP
    digitalWrite(PIN_LED2, LOW);
  }
  else
  {
    error = FLASHER_EBVALUE;
  }

  return error;
}

/* Starts the ADT7310 temperature measurement (16-bit mode) */
uint16_t flasher_START_TEMPERATURE()
{
  uint16_t error = 0;

  start_temperature(SPI_1, ADT7310_CS);

  return error;
}

/* Reads the ADT7310 temperature (16-bit mode) */
float flasher_READ_TEMPERATURE(uint16_t *error)
{
  float temperature;

  temperature = read_temperature(SPI_1, ADT7310_CS);
  *error = 0;

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
  struct {
    union {
      int16_t temperature; // Little endian
      struct {
        uint8_t lo_byte;
        uint8_t hi_byte;
      } bytes;
    };
  } temp_reg;
  float temperature;

  // Read temperature from this SPI using this CS
  this_spi.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3)); // ADT7310 needs SPI Mode 3
  digitalWrite(this_cs,LOW); // Pull CS low
  this_spi.transfer(0x50); // Send command byte: Read, Register 2 (temperature value register (16-bit))
  temp_reg.bytes.hi_byte = this_spi.transfer(0xff); // Read MSB
  temp_reg.bytes.lo_byte = this_spi.transfer(0xff); // Read LSB
  digitalWrite(this_cs,HIGH); // Pull CS high
  this_spi.endTransaction();

  // Convert the temperature to float
  temperature = (float)temp_reg.temperature;
  temperature = temperature / 128.0;

  return temperature;
}

uint16_t flasher_SET_LED_CURRENT(uint8_t current)
// Configures the LED current
{
  uint16_t error = 0;

  if (current > 0x0F)
  {
    error = FLASHER_EBVALUE;
    return error;
  }

  digitalWrite(LED_A0, ((current >> 0) & 0x01));
  digitalWrite(LED_A1, ((current >> 1) & 0x01));
  digitalWrite(LED_A2, ((current >> 2) & 0x01));
  digitalWrite(LED_A3, ((current >> 3) & 0x01));

  return error;
}

uint16_t flasher_SET_PULSE_WIDTH(uint8_t width)
// Configures the pulse width (DS1023 delay)
{
  uint16_t error = 0;

  digitalWrite(DS1023_LE, LOW); // Make sure the LE is low
  digitalWrite(DS1023_CLK, LOW); // Make sure the CLK is low

  digitalWrite(DS1023_LE, HIGH); // Raise the LE

  for (int i = 7; i >= 0; i--) // Clock the width bits out MS bit first
  {
    digitalWrite(DS1023_D, ((width >> i) & 0x01)); // Set the data bit
    digitalWrite(DS1023_D, ((width >> i) & 0x01)); // NOP
    digitalWrite(DS1023_CLK, HIGH); // Raise the CLK
    digitalWrite(DS1023_CLK, HIGH); // NOP
    digitalWrite(DS1023_CLK, LOW); // Lower the CLK
    digitalWrite(DS1023_CLK, LOW); // NOP
  }

  digitalWrite(DS1023_LE, LOW); // Make sure the LE is low

  return error;
}

uint16_t flasher_READ_SERIAL_NO(uint8_t *serial_no)
// Read the DS28CM00 serial number via Wire
// The six serial number bytes are copied to serial_no LSB first (little endian)
// TO DO: implement the X^8 + X^5 + X^4 + 1 CRC
{
  uint16_t error = 0;

  Wire.beginTransmission(0x50); // Start communication with the DS28CM00
  Wire.write(0x00); // Point to address 0 (family code)
  Wire.endTransmission(false); // Restart

  Wire.requestFrom(0x50, 8); // Request 8 bytes

  if (Wire.available() < 8)
  {
    error = FLASHER_ESERIALNO;
    return error;
  }

  char c = Wire.read(); // Read the first byte (family code)

  if (c != 0x70) // Check that the family code is correct
  {
    error = FLASHER_ESERIALNO;
    return error;
  }

  for (int i = 0; i < 6; i++) // Read the six serial number bytes (LSB is read first)
  {
    c = Wire.read(); // Read the byte
    *(serial_no + i) = c; // Store the byte
    // TO DO: implement the X^8 + X^5 + X^4 + 1 CRC
  }

  c = Wire.read(); // Read the CRC byte
  // TO DO: implement the X^8 + X^5 + X^4 + 1 CRC

  return error;
}

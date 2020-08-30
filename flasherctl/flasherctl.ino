#include "flasher.h"
#include "iostack.h"

#include <SPI.h>
// Define the SPIClass for the I2C/SPI Header using SERCOM0
SPIClass SPI_0 (&PERIPH_SPI, PIN_SPI_MISO, PIN_SPI_SCK, PIN_SPI_MOSI, PAD_SPI_TX, PAD_SPI_RX);
// Define the SPIClass for the ADT7310 Temperature Sensor on the LED Board using SERCOM1
SPIClass SPI_1 (&PERIPH_SPI1, PIN_SPI1_MISO, PIN_SPI1_SCK, PIN_SPI1_MOSI, PAD_SPI1_TX, PAD_SPI1_RX);
// Define the SPIClass for the W5500 using SERCOM4
SPIClass SPI_2 (&PERIPH_SPI2, PIN_SPI2_MISO, PIN_SPI2_SCK, PIN_SPI2_MOSI, PAD_SPI2_TX, PAD_SPI2_RX);

#include <Wire.h> // Needed for the DS28CM00 serial number

// Network configuration
static const uint16_t udp_listen_port = 512;
static uint8_t udp_socket = 0xff;

// Command codes (to match class FlasherCommand in flasherctl.py)
enum flasherctl_cmd_code {CMD_LED_BUILTIN = 0x0000,
                          CMD_START_TEMPERATURE,
                          CMD_READ_TEMPERATURE,
                          CMD_INIT_FLASHER,
                          CMD_READ_SERIAL_NO,
                          CMD_SET_LED_CURRENT,
                          CMD_SET_PULSE_WIDTH,
                          CMD_TEST_PULSE,
                          CMD_READ_TRIG,
                          CMD_REPORT_FLASHERCTL_ERR=0xffff};

// Command handlers
enum iostack_error_code flasherctl_LED_BUILTIN(struct iostack_request *request);
enum iostack_error_code flasherctl_START_TEMPERATURE(struct iostack_request *request);
enum iostack_error_code flasherctl_READ_TEMPERATURE(struct iostack_request *request);
enum iostack_error_code flasherctl_READ_SERIAL_NO(struct iostack_request *request);
enum iostack_error_code flasherctl_SET_LED_CURRENT(struct iostack_request *request);
enum iostack_error_code flasherctl_SET_PULSE_WIDTH(struct iostack_request *request);
enum iostack_error_code flasherctl_TEST_PULSE(struct iostack_request *request);

// Command definitions
static struct iostack_cmd flasher_cmds[] = {{CMD_LED_BUILTIN, flasherctl_LED_BUILTIN},
                                            {CMD_START_TEMPERATURE, flasherctl_START_TEMPERATURE},
                                            {CMD_READ_TEMPERATURE, flasherctl_READ_TEMPERATURE},
                                            {CMD_READ_SERIAL_NO, flasherctl_READ_SERIAL_NO},
                                            {CMD_SET_LED_CURRENT, flasherctl_SET_LED_CURRENT},
                                            {CMD_SET_PULSE_WIDTH, flasherctl_SET_PULSE_WIDTH},
                                            {CMD_TEST_PULSE, flasherctl_TEST_PULSE}};

static struct iostack_subsystem flasher_subsystem = {.id = SYS_FLASHER, .cmds = flasher_cmds};

void setup()
{
  // Initialise the I/O pins
  pinMode(LED_BUILTIN, OUTPUT); // Red LED
  digitalWrite(LED_BUILTIN, LOW); // Turn the LED off
  pinMode(PIN_LED2, OUTPUT); // RX_LED / TEST_PULSE
  digitalWrite(PIN_LED2, LOW); // Make TEST_PULSE low

  pinMode(SS, OUTPUT); // CS for the W5500
  digitalWrite(SS, HIGH);

  pinMode(ADT7310_CS, OUTPUT); // CS for the ADT7310
  digitalWrite(ADT7310_CS, HIGH);

  // Pins for the DS1023-25
  // 8-bit data, MSB (D7) first
  // Minimum CLK pulse width: 50ns
  pinMode(DS1023_LE, OUTPUT); // Latch Enable for the DS1023
  digitalWrite(DS1023_LE, LOW); // Only raise LE when clocking in a new delay value
  pinMode(DS1023_CLK, OUTPUT); // Clock for the DS1023
  digitalWrite(DS1023_CLK, LOW); // Data pin is read on the rising edge of the clock
  pinMode(DS1023_D, OUTPUT); // Serial data input for the DS1023
  digitalWrite(DS1023_D, LOW);

  // LED Current: default to minimum brightness (A0-A3 LOW)
  pinMode(LED_A0, OUTPUT);
  digitalWrite(LED_A0, LOW);
  pinMode(LED_A1, OUTPUT);
  digitalWrite(LED_A1, LOW);
  pinMode(LED_A2, OUTPUT);
  digitalWrite(LED_A2, LOW);
  pinMode(LED_A3, OUTPUT);
  digitalWrite(LED_A3, LOW);

  // Wait for Vcc to stabilise
  delay(2000);

  // Initialise the SPI ports
  SPI_2.begin(); // Ethernet
  SPI_2.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); // SPI_2 is always in mode 0

  // The mode for SPI_1 is set by the appropriate beginTransaction
  SPI_1.begin(); // ADT7310 Temperature sensor

  // Start I2C for the DS28CM00 serial number
  Wire.begin();

  // Initialise the temperature sensor - to stop the continuous conversions
  flasher_START_TEMPERATURE();

  // Initialise subsystems
  // - serial communication via USB
  Serial.begin(115200);
  delay(10);
  Serial.println("Flasher");
  // - w5500 & I/O stack
  Serial.println("Initialising I/O stack...");
  udp_socket = iostack_init(udp_listen_port);

  if (udp_socket >= W5500_NUM_SOCKETS) {
    Serial.println(F("failed; cannot continue"));
    while (1)
      ;
  }

  Serial.println(F("  registering flasher subsystem..."));
  iostack_register_commands(&flasher_subsystem, flasher_cmds,
                            sizeof(flasher_cmds) / sizeof(*flasher_cmds));
  iostack_register_subsystem(&flasher_subsystem);

}


void loop()
{
  iostack_tick(udp_socket);
}


void flasherctl_send_error(struct iostack_request *request, uint16_t error)
{
  iostack_response_begin(request, CMD_REPORT_FLASHERCTL_ERR);
  iostack_response_write(request, &error, sizeof(error));
  iostack_response_end(request);
}


void flasherctl_send_acknowledge(struct iostack_request *request)
{
  iostack_response_begin(request, request->request_code);
  iostack_response_end(request);
}


// Turns the controller LED pin on or off
enum iostack_error_code flasherctl_LED_BUILTIN(struct iostack_request *request)
{
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *on_off = (uint8_t *) request->payload;
  uint16_t error = flasher_LED_BUILTIN(*on_off);

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_START_TEMPERATURE(struct iostack_request *request)
{
  if (request->size != 0)
    return IOSTACK_ERR_INVALID_SIZE;

  uint16_t error = flasher_START_TEMPERATURE();

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_READ_TEMPERATURE(struct iostack_request *request)
{
  if (request->size != 0)
    return IOSTACK_ERR_INVALID_SIZE;

  uint16_t error = 0;
  float val = flasher_READ_TEMPERATURE(&error);

  if (error) {
    flasherctl_send_error(request, error);
  } else {
    iostack_response_begin(request, request->request_code);
    iostack_response_write(request, &val, sizeof(val));
    iostack_response_end(request);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_READ_SERIAL_NO(struct iostack_request *request)
{
  if (request->size != 0)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t serial_no[6];
  uint8_t *serial_ptr = serial_no;
  uint16_t error = flasher_READ_SERIAL_NO(serial_ptr);

  if (error) {
    flasherctl_send_error(request, error);
  } else {
    iostack_response_begin(request, request->request_code);
    iostack_response_write(request, &serial_no, sizeof(serial_no));
    iostack_response_end(request);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_SET_LED_CURRENT(struct iostack_request *request)
{
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *current = (uint8_t *) request->payload;
  uint16_t error = flasher_SET_LED_CURRENT((uint8_t) *current);

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_SET_PULSE_WIDTH(struct iostack_request *request)
{
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *width = (uint8_t *) request->payload;
  uint16_t error = flasher_SET_PULSE_WIDTH((uint8_t) *width);

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_TEST_PULSE(struct iostack_request *request)
{
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *on_off = (uint8_t *) request->payload;
  uint16_t error = flasher_TEST_PULSE((uint8_t) *on_off);

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

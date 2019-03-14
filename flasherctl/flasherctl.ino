#include "flasher.h"
#include "iostack.h"

// Define the SPIClass for the W5500 using SERCOM4
#include <SPI.h>
SPIClass SPI_0 (&PERIPH_SPI, PIN_SPI_MISO, PIN_SPI_SCK, PIN_SPI_MOSI, PAD_SPI_TX, PAD_SPI_RX);
// Define the SPIClass for the four timing boards
SPIClass SPI_1 (&PERIPH_SPI1, PIN_SPI1_MISO, PIN_SPI1_SCK, PIN_SPI1_MOSI, PAD_SPI1_TX, PAD_SPI1_RX);
SPIClass SPI_2 (&PERIPH_SPI2, PIN_SPI2_MISO, PIN_SPI2_SCK, PIN_SPI2_MOSI, PAD_SPI2_TX, PAD_SPI2_RX);
SPIClass SPI_3 (&PERIPH_SPI3, PIN_SPI3_MISO, PIN_SPI3_SCK, PIN_SPI3_MOSI, PAD_SPI3_TX, PAD_SPI3_RX);
SPIClass SPI_4 (&PERIPH_SPI4, PIN_SPI4_MISO, PIN_SPI4_SCK, PIN_SPI4_MOSI, PAD_SPI4_TX, PAD_SPI4_RX);

// Network configuration
static const uint16_t udp_listen_port = 512;
static uint8_t udp_socket = 0xff;

// Command codes (to match class FlasherCommand in flasherctl.py)
enum flasherctl_cmd_code {CMD_LED_BUILTIN = 0x0000,
                          CMD_START_TEMPERATURE,
                          CMD_READ_TEMPERATURE,
                          CMD_INIT_FLASHER,
                          CMD_SET_GPO_PIN,
                          CMD_READ_SERIAL_NO,
                          CMD_SET_LEDS,
                          CMD_SET_PULSE_WIDTH,
                          CMD_TEST_PULSE,
                          CMD_READ_TRIG,
                          CMD_REPORT_FLASHERCTL_ERR=0xffff};

// Command handlers
enum iostack_error_code flasherctl_LED_BUILTIN(struct iostack_request *request);
enum iostack_error_code flasherctl_START_TEMPERATURE(struct iostack_request *request);
enum iostack_error_code flasherctl_READ_TEMPERATURE(struct iostack_request *request);
enum iostack_error_code flasherctl_INIT_FLASHER(struct iostack_request *request);
enum iostack_error_code flasherctl_SET_GPO_PIN(struct iostack_request *request);
enum iostack_error_code flasherctl_READ_SERIAL_NO(struct iostack_request *request);
enum iostack_error_code flasherctl_SET_LEDS(struct iostack_request *request);
enum iostack_error_code flasherctl_SET_PULSE_WIDTH(struct iostack_request *request);
enum iostack_error_code flasherctl_TEST_PULSE(struct iostack_request *request);
enum iostack_error_code flasherctl_READ_TRIG(struct iostack_request *request);

// Command definitions
static struct iostack_cmd flasher_cmds[] = {{CMD_LED_BUILTIN, flasherctl_LED_BUILTIN},
                                            {CMD_START_TEMPERATURE, flasherctl_START_TEMPERATURE},
                                            {CMD_READ_TEMPERATURE, flasherctl_READ_TEMPERATURE},
                                            {CMD_INIT_FLASHER, flasherctl_INIT_FLASHER},
                                            {CMD_SET_GPO_PIN, flasherctl_SET_GPO_PIN},
                                            {CMD_READ_SERIAL_NO, flasherctl_READ_SERIAL_NO},
                                            {CMD_SET_LEDS, flasherctl_SET_LEDS},
                                            {CMD_SET_PULSE_WIDTH, flasherctl_SET_PULSE_WIDTH},
                                            {CMD_TEST_PULSE, flasherctl_TEST_PULSE},
                                            {CMD_READ_TRIG, flasherctl_READ_TRIG}};

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
  pinMode(CS1_1, OUTPUT); // CSs for Timing Board 1
  digitalWrite(CS1_1, HIGH);
  pinMode(CS2_1, OUTPUT);
  digitalWrite(CS2_1, HIGH);
  pinMode(CS1_2, OUTPUT); // CSs for Timing Board 2
  digitalWrite(CS1_2, HIGH);
  pinMode(CS2_2, OUTPUT);
  digitalWrite(CS2_2, HIGH);
  pinMode(CS1_3, OUTPUT); // CSs for Timing Board 3
  digitalWrite(CS1_3, HIGH);
  pinMode(CS2_3, OUTPUT);
  digitalWrite(CS2_3, HIGH);
  pinMode(CS1_4, OUTPUT); // CSs for Timing Board 4
  digitalWrite(CS1_4, HIGH);
  pinMode(CS2_4, OUTPUT);
  digitalWrite(CS2_4, HIGH);
  
  // Wait for Vcc to stabilise
  delay(2000);

  // Initialise the SPI ports
  SPI_0.begin(); // Ethernet
  SPI_0.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); // SPI_0 is always in mode 0
  
  // The mode for SPI_1 - SPI_4 is set by the appropriate beginTransaction
  SPI_1.begin(); // Timing Board 0
  SPI_2.begin(); // Timing Board 1
  SPI_3.begin(); // Timing Board 2
  SPI_4.begin(); // Timing Board 3

  // Initialise the timing board I/O pins
  flasher_INIT_FLASHER((uint8_t)0);
  flasher_INIT_FLASHER((uint8_t)1);
  flasher_INIT_FLASHER((uint8_t)2);
  flasher_INIT_FLASHER((uint8_t)3);

  // Initialise the temperature sensors - to stop the continuous conversions
  flasher_START_TEMPERATURE((uint8_t)0);
  flasher_START_TEMPERATURE((uint8_t)1);
  flasher_START_TEMPERATURE((uint8_t)2);
  flasher_START_TEMPERATURE((uint8_t)3);

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
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *board = (uint8_t *) request->payload;
  uint16_t error = flasher_START_TEMPERATURE((uint8_t) *board);

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_READ_TEMPERATURE(struct iostack_request *request)
{
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *board = (uint8_t *) request->payload;
  uint16_t error = 0;
  float val = flasher_READ_TEMPERATURE((uint8_t) *board, &error);

  if (error) {
    flasherctl_send_error(request, error);
  } else {
    iostack_response_begin(request, request->request_code);
    iostack_response_write(request, &val, sizeof(val));
    iostack_response_end(request);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_INIT_FLASHER(struct iostack_request *request)
{
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *board = (uint8_t *) request->payload;
  uint16_t error = flasher_INIT_FLASHER((uint8_t) *board);

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_SET_GPO_PIN(struct iostack_request *request)
{
  if (request->size != 3)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *board = (uint8_t *) request->payload;
  uint8_t *pin = (uint8_t *) request->payload + 1;
  uint8_t *on_off = (uint8_t *) request->payload + 2;
  uint16_t error = flasher_SET_GPO_PIN((uint8_t) *board, (uint8_t) *pin, (uint8_t) *on_off);

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_READ_SERIAL_NO(struct iostack_request *request)
{
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *board = (uint8_t *) request->payload;
  uint8_t serial_no[6];
  uint8_t *serial_ptr = serial_no;
  uint16_t error = flasher_READ_SERIAL_NO((uint8_t) *board, serial_ptr);

  if (error) {
    flasherctl_send_error(request, error);
  } else {
    iostack_response_begin(request, request->request_code);
    iostack_response_write(request, &serial_no, sizeof(serial_no));
    iostack_response_end(request);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_SET_LEDS(struct iostack_request *request)
{
  if (request->size != 3)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *board = (uint8_t *) request->payload;
  uint8_t *lo_leds = (uint8_t *) request->payload + 1;
  uint8_t *hi_leds = (uint8_t *) request->payload + 2;
  uint16_t error = flasher_SET_LEDS((uint8_t) *board, (uint8_t) *hi_leds, (uint8_t) *lo_leds);

  if (error == 0) {
    flasherctl_send_acknowledge(request);
  } else {
    flasherctl_send_error(request, error);
  }

  return IOSTACK_ERR_OKAY;
}

enum iostack_error_code flasherctl_SET_PULSE_WIDTH(struct iostack_request *request)
{
  if (request->size != 2)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *board = (uint8_t *) request->payload;
  uint8_t *width = (uint8_t *) request->payload + 1;
  uint16_t error = flasher_SET_PULSE_WIDTH((uint8_t) *board, (uint8_t) *width);

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

enum iostack_error_code flasherctl_READ_TRIG(struct iostack_request *request)
{
  if (request->size != 1)
    return IOSTACK_ERR_INVALID_SIZE;

  uint8_t *board = (uint8_t *) request->payload;
  uint8_t trig[1];
  uint8_t *trig_ptr = trig;
  uint16_t error = flasher_READ_TRIG((uint8_t) *board, trig_ptr);

  if (error) {
    flasherctl_send_error(request, error);
  } else {
    iostack_response_begin(request, request->request_code);
    iostack_response_write(request, &trig, sizeof(trig));
    iostack_response_end(request);
  }

  return IOSTACK_ERR_OKAY;
}







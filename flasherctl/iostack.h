#ifndef __IOSTACK_H__
#define __IOSTACK_H__

// flasher iostack header

// See:
// https://learn.adafruit.com/adafruit-feather-m0-basic-proto/adapting-sketches-to-m0#aligned-memory-access-7-13
// http://forum.arduino.cc/index.php?topic=184916.0
// for an explanation of struct __attribute__((packed)) 

#include <Arduino.h>

#include "w5500.h"

#ifdef __cplusplus
extern "C" {
#endif

// Subsystem ID for the iostack
#define SYS_IOSTACK 0x00

static const uint16_t iostack_header_size = 5;
static const uint16_t iostack_max_payload_size = 256 - iostack_header_size;
static const uint32_t iostack_eeprom_magic = 0x10574c6b;
static const uint16_t iostack_eeprom_base = 0x0000;

struct __attribute__((packed)) iostack_config {
  // Header
  uint32_t magic;
  uint16_t version;
  uint16_t checksum;

  // Configuration structures
  struct w5500_config static_ethernet_config;
};

enum iostack_error_code {
  IOSTACK_ERR_OKAY = 0x0000,
  IOSTACK_ERR_UNKNOWN_SUBSYSTEM,
  IOSTACK_ERR_UNKNOWN_COMMAND,
  IOSTACK_ERR_INVALID_SIZE,
  IOSTACK_ERR_INVALID_REGISTER,
  IOSTACK_ERR_INVALID_MAC,
  IOSTACK_ERR_UNHANDLED_ERROR,
};

typedef enum iostack_error_code iostack_handler(
    struct iostack_request *request);

struct __attribute__((packed)) iostack_cmd {
  uint8_t code;
  iostack_handler *handler;

  struct iostack_cmd *prev;
  struct iostack_cmd *next;
};

struct __attribute__((packed)) iostack_subsystem {
  uint8_t id;
  struct iostack_cmd *cmds;

  struct iostack_subsystem *prev;
  struct iostack_subsystem *next;
};

struct __attribute__((packed)) iostack_request {
  uint16_t id;
  uint8_t subsystem_id;
  union {
    uint16_t request_code;
    uint16_t response_code;
  };

  uint8_t payload[iostack_max_payload_size];
  uint16_t size;
  uint8_t response_state;

  uint8_t socket;
  struct w5500_udp_header udp_header;
};

int8_t iostack_init(uint16_t udp_listen_port);
void iostack_register_commands(struct iostack_subsystem *subsystem,
                               struct iostack_cmd *cmds, uint8_t ncmds);
void iostack_register_subsystem(struct iostack_subsystem *subsystem);
void iostack_tick(uint8_t udp_socket);

uint8_t iostack_response_begin(struct iostack_request *request,
                               uint16_t response_code);
uint16_t iostack_response_write(struct iostack_request *request, void *src,
                                uint16_t size);
uint8_t iostack_response_end(struct iostack_request *request);

void eeprom_update_block(struct iostack_config *cfg, uint16_t base, size_t size_of);
void eeprom_read_block(struct iostack_config *cfg, uint16_t base, size_t size_of);

#ifdef __cplusplus
}
#endif

#endif

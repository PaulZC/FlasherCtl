#include "iostack.h"

// Updated to use https://github.com/FrankBoesing/FastCRC
#include <FastCRC.h>
FastCRC16 CRC16;

// Updated to use https://github.com/cmaglie/FlashStorage
#include <FlashAsEEPROM.h>

// Handlers
enum iostack_error_code iostack_handle_register_read(struct iostack_request *request);
enum iostack_error_code iostack_handle_register_write(struct iostack_request *request);
enum iostack_error_code iostack_handle_ping(struct iostack_request *request);

// Special handlers
enum iostack_error_code iostack_handle_ethernet_configuration_read(struct iostack_request *request);
enum iostack_error_code iostack_handle_ethernet_configuration_write(struct iostack_request *request, uint8_t *payload, uint16_t size);

// Random Locally Administered Unicast MAC Addresses:
// https://www.hellion.org.uk/cgi-bin/randmac.pl?scope=local&type=unicast
// ee:ce:31:07:b0:e4
// 96:8f:76:c0:da:cf
// be:6c:c0:04:a0:51
// 62:a2:09:a4:06:13

// Configuration
struct iostack_config iostack_default_config = {
  .magic = iostack_eeprom_magic,
  .version = 0,
  .checksum = 0,  // requires update
  .static_ethernet_config = {
    .gateway_addr = {192, 168, 0, 1},
    .subnet_mask = {255, 255, 252, 0},
    .mac_address = {0, 0, 0, 0, 0, 0},  // requires update
    .ip_address = {192, 168, 0, 200}
  }
};

struct iostack_config iostack_config;

// Commands
enum iostack_cmd_code {CMD_READ_REG=0x0000, CMD_WRITE_REG, CMD_PING, CMD_REPORT_ERR=0xffff};

enum iostack_reg {REG_ETH_CFG=0x0000};

static struct iostack_cmd iostack_cmds[] =
  {{CMD_READ_REG, iostack_handle_register_read},
   {CMD_WRITE_REG, iostack_handle_register_write},
   {CMD_PING, iostack_handle_ping}};

// Subsystems
static struct iostack_subsystem iostack_subsystem = {
  .id = SYS_IOSTACK,
  .cmds = iostack_cmds,
  .prev = NULL,
  .next = NULL
};

struct iostack_subsystem *iostack_subsystems = &iostack_subsystem;


uint16_t iostack_calculate_checksum(struct iostack_config *cfg)
{
  uint8_t *data = (uint8_t *) &cfg->checksum + 2;
  uint16_t size = sizeof(struct iostack_config) -
                  ((uint8_t *) &cfg->checksum + 2 - (uint8_t *) cfg);

  uint16_t crc = CRC16.x25(data, size);

  return crc;
}

void eeprom_update_block(struct iostack_config *cfg, uint16_t base, size_t size_of)
{
  uint8_t value;
  int address;
  for (size_t count = 0; count < size_of; count++) {
    value = ((uint8_t *) cfg)[count];
    address = count + base;
    EEPROM.write(address, value);
  }
  EEPROM.commit();
}

void eeprom_read_block(struct iostack_config *cfg, uint16_t base, size_t size_of)
{
  uint8_t value;
  int address;
  for (size_t count = 0; count < size_of; count++) {
    address = count + base;
    value = EEPROM.read(address);
    ((uint8_t *) cfg)[count] = value;
  }
}

void iostack_update_eeprom(void)
{
  iostack_config.checksum = iostack_calculate_checksum(&iostack_config);
  eeprom_update_block(&iostack_config, iostack_eeprom_base,
                      sizeof(struct iostack_config));
}


void iostack_query_mac_address(uint8_t mac[6])
{
  // TODO: Might need a long global timeout which is reset with each keystroke
  // and issues a reset (Watchdog)
  static const char query[] = "\n  please enter MAC address: ";

  uint8_t nchars = 0;
  while (1) {
//    for (uint8_t prompt = 0; prompt < 2; prompt++) {
      Serial.print(query);

      for (uint8_t i = 0; i < nchars; ++i) {
        if (i % 2 == 0)
          Serial.print((mac[i / 2] & 0xf0) >> 4, HEX);
        else
          Serial.print(mac[i / 2] & 0x0f, HEX);
  
        if (i % 2 && i < 11)
          Serial.print(":");
      }
      
//      if (!prompt) {
        for (uint8_t i = nchars; i < 12; i++) {
          Serial.print("?");
          if (i % 2 && i < 11)
            Serial.print(":");
        }
//      }
//    }

    uint16_t timeout = 1000;
    while (!Serial.available() && timeout-- > 0)
      delay(1);

    if (!Serial.available())
      continue;

    uint8_t c = Serial.read();
    if (c == 0x7f) {
      // Handle backspace: remove last byte
      if (nchars)
        nchars--;

      continue;
    } else if ((c == 0x0d || c == 0x0a) && nchars == 12) {
      // Accept MAC
      break;
    } else if (nchars < 12) {
      // Read next character
      if (c >= '0' && c <= '9')
        c = c - '0';
      else if (c >= 'a' && c <= 'f')
        c = 10 + c - 'a';
      else if (c >= 'A' && c <= 'F')
        c = 10 + c - 'A';
      else
        continue;

      if (nchars % 2 == 0)
        mac[nchars / 2] = c << 4;
      else
        mac[nchars / 2] |= c;

      nchars++;
    }
  }

  Serial.println();
}


void iostack_register_commands(struct iostack_subsystem *subsystem,
                               struct iostack_cmd *cmds, uint8_t ncmds)
{
  if (!subsystem)
    return;

  if (!cmds || !ncmds) {
    cmds = NULL;
    ncmds = 0;
  }

  subsystem->cmds = cmds;

  for (uint8_t i = 0; i < ncmds; ++i) {
    cmds[i].prev = i > 0 ? &cmds[i - 1] : NULL;
    cmds[i].next = i < ncmds ? &cmds[i + 1] : NULL;
  }
}


int8_t iostack_init(uint16_t udp_listen_port)
{
  // Initialise the commands linked list
  iostack_register_commands(&iostack_subsystem, iostack_cmds,
                            sizeof(iostack_cmds) / sizeof(*iostack_cmds));

  // Read EEPROM
  Serial.print(F("  validating EEPROM configuration... "));
  eeprom_read_block(&iostack_config, iostack_eeprom_base,
                    sizeof(struct iostack_config));

  // Validate content
  if (iostack_config.magic != iostack_eeprom_magic ||
      iostack_calculate_checksum(&iostack_config) != iostack_config.checksum) {
    Serial.println(F("invalid"));

    // Generate default configuration
    iostack_config = iostack_default_config;

    // Query MAC address via serial connection
    iostack_query_mac_address(
        iostack_config.static_ethernet_config.mac_address);

    // Store in EEPROM
    Serial.println(F("  storing configuration in EEPROM..."));
    iostack_update_eeprom();
  } else {
    Serial.println(F("ok"));
  }

  Serial.print(F("  MAC address (in EEPROM) is: "));
  for (uint8_t i = 0; i < 12; ++i) {
    if (i % 2 == 0)
      Serial.print((iostack_config.static_ethernet_config.mac_address[i / 2] & 0xf0) >> 4, HEX);
    else
      Serial.print(iostack_config.static_ethernet_config.mac_address[i / 2] & 0x0f, HEX);

    if (i % 2 && i < 11)
      Serial.print(":");
  }
  Serial.println();

  // Configure W5500
  Serial.println(F("  initialising W5500..."));
  w55_init();
  Serial.println(F("  configuring W5500..."));
  if (w55_config(&iostack_config.static_ethernet_config)) {
    Serial.println(F("  configuration failed; cannot continue"));

    // TODO: Schedule reset via Watchdog
    while (1)
      ;
  }

  // Open UDP socket
  Serial.println(F("  opening UDP socket..."));
  uint8_t udp_socket = w55_udp_open(udp_listen_port);

  Serial.print(F("  listening on "));
  Serial.print(iostack_config.static_ethernet_config.ip_address[0]);
  Serial.print(F("."));
  Serial.print(iostack_config.static_ethernet_config.ip_address[1]);
  Serial.print(F("."));
  Serial.print(iostack_config.static_ethernet_config.ip_address[2]);
  Serial.print(F("."));
  Serial.print(iostack_config.static_ethernet_config.ip_address[3]);
  Serial.print(F(":"));
  Serial.println(udp_listen_port);

  // TODO: Propagate errors
  return udp_socket;
}


uint8_t iostack_response_begin(struct iostack_request *request,
                               uint16_t response_code)
{
  if (request->response_state != 0)
    return 1;

  request->response_code = response_code;

  if (w55_udp_begin(request->socket, &request->udp_header) ||
      w55_udp_write(request->socket, (uint8_t *) request,
                    iostack_header_size) != iostack_header_size)
    return 1;

  request->response_state = 1;
  return 0;
}


uint16_t iostack_response_write(struct iostack_request *request, void *src,
                                uint16_t size)
{
  if (request->response_state != 1)
    return 0;

  return w55_udp_write(request->socket, (uint8_t *) src, size);
}


uint8_t iostack_response_end(struct iostack_request *request)
{
  if (request->response_state != 1)
    return 1;

  request->response_state = 2;
  return w55_udp_end(request->socket);
}


enum iostack_error_code iostack_handle_ping(struct iostack_request *request)
{
  Serial.println(F("ping request received"));

  iostack_response_begin(request, request->request_code);
  iostack_response_write(request, request->payload, request->size);
  iostack_response_end(request);

  // FIXME
  return IOSTACK_ERR_OKAY;
}


enum iostack_error_code iostack_handle_register_read(
    struct iostack_request *request)
{
  if (request->size != 2)
    return IOSTACK_ERR_INVALID_SIZE;

  uint16_t reg = (uint16_t)(request->payload[0] << 8) + (uint16_t) request->payload[1];

  Serial.print(F("register read request received for register "));
  Serial.println(reg);

  switch (reg) {
    case REG_ETH_CFG:
      return iostack_handle_ethernet_configuration_read(request);

    default:
      return IOSTACK_ERR_INVALID_REGISTER;
  }
}


enum iostack_error_code iostack_handle_register_write(
    struct iostack_request *request)
{
  if (request->size < 3)
    return IOSTACK_ERR_INVALID_SIZE;

  uint16_t reg = (uint16_t)(request->payload[0] << 8) + (uint16_t) request->payload[1];
  uint8_t *payload = &request->payload[2];
  uint16_t size = request->size - 2;

  Serial.print(F("register write request received for register "));
  Serial.println(reg);

  switch (reg) {
    case REG_ETH_CFG:
      return iostack_handle_ethernet_configuration_write(request, payload,
                                                         size);

    default:
      return IOSTACK_ERR_INVALID_REGISTER;
  }
}


enum iostack_error_code iostack_handle_ethernet_configuration_read(
    struct iostack_request *request)
{
  // Read configuration from chip
  w55_readn(W5500_GAR, W5500_BLB_COM,
            (uint8_t *) &iostack_config.static_ethernet_config,
            sizeof(struct w5500_config));

  // Temporarily adjust subnet to include target
  // TODO: Determine if this is needed using dest_ip AND subnet_mask
  uint8_t i = 4;
  uint8_t old_subnet_mask[4];
  while (i--) {
    old_subnet_mask[i] = iostack_config.static_ethernet_config.subnet_mask[i];
    iostack_config.static_ethernet_config.subnet_mask[i] = 0;
  }
  w55_config(&iostack_config.static_ethernet_config);

  i = 4;
  while (i--)
    iostack_config.static_ethernet_config.subnet_mask[i] = old_subnet_mask[i];

  // Reply
  // FIXME
  iostack_response_begin(request, request->request_code);
  iostack_response_write(request, &iostack_config.static_ethernet_config,
                         sizeof(struct w5500_config));
  iostack_response_end(request);

  // Restore previous subnet mask
  w55_config(&iostack_config.static_ethernet_config);

  // TODO: integration with tick loop
  // FIXME
  return IOSTACK_ERR_OKAY;
}


static void iostack_send_acknowledge(struct iostack_request *request,
                                     uint16_t code)
{
  request->subsystem_id = 0;
  iostack_response_begin(request, request->request_code);
  iostack_response_write(request, &code, sizeof(code));
  iostack_response_end(request);
}


static void iostack_send_error(struct iostack_request *request,
                               uint16_t error_code)
{
  request->subsystem_id = 0;
  iostack_response_begin(request, 0xff);
  iostack_response_write(request, &error_code, sizeof(error_code));
  iostack_response_end(request);
}


enum iostack_error_code iostack_handle_ethernet_configuration_write(
    struct iostack_request *request, uint8_t *payload, uint16_t size)
{
  if (size != 18)
    return IOSTACK_ERR_INVALID_SIZE;

  w5500_config *new_ethernet_config = (w5500_config *) payload;

  // Check MAC address to see whether the new configuration is intended for this
  // device
  uint8_t i = 6;
  while (i--)
    if (new_ethernet_config->mac_address[i] !=
        iostack_config.static_ethernet_config.mac_address[i]) {
      iostack_send_error(request, IOSTACK_ERR_INVALID_MAC);
      return IOSTACK_ERR_INVALID_MAC;
    }

  // Write configuration to chip and read back
  w55_config(new_ethernet_config);
  w55_readn(W5500_GAR, W5500_BLB_COM,
            (uint8_t *) &iostack_config.static_ethernet_config,
            sizeof(struct w5500_config));

  // Update EEPROM
  iostack_update_eeprom();

  iostack_send_acknowledge(request, IOSTACK_ERR_OKAY);

  // FIXME
  return IOSTACK_ERR_OKAY;
}


/* Assumptions which simplify the code:
    - no duplicate ids
*/
void iostack_register_subsystem(struct iostack_subsystem *subsystem)
{
  if (!subsystem || !iostack_subsystems)
    return;

  struct iostack_subsystem *cur = iostack_subsystems;
  while (cur->next)
    cur = cur->next;

  cur->next = subsystem;
  subsystem->prev = cur;
  subsystem->next = NULL;
}


void iostack_tick(uint8_t udp_socket)
{
  static struct iostack_request request;
  request.response_state = 0;

  uint16_t nbytes =
      w55_udp_read(udp_socket, &request.udp_header, (uint8_t *) &request,
                   iostack_header_size + iostack_max_payload_size);

  if (nbytes == 0)
    return;

  if (nbytes < iostack_header_size) {
    Serial.println("request too small");
    return;
  }

  request.size = nbytes - iostack_header_size;

  // Find subsystem
  struct iostack_subsystem *subsystem = iostack_subsystems;
  while (subsystem && subsystem->id != request.subsystem_id)
    subsystem = subsystem->next;

  if (!subsystem) {
    Serial.println("received request for unknown subsystem");
    iostack_send_error(&request, IOSTACK_ERR_UNKNOWN_SUBSYSTEM);
    return;
  }

  // Find command
  struct iostack_cmd *cmd = subsystem->cmds;
  while (cmd && cmd->code != request.request_code)
    cmd = cmd->next;

  if (!cmd) {
    Serial.println("received request with unknown command");
    iostack_send_error(&request, IOSTACK_ERR_UNKNOWN_COMMAND);
    return;
  }

  // Execute command and handle return code
  enum iostack_error_code rc = cmd->handler(&request);
  if (rc != IOSTACK_ERR_OKAY) {
    Serial.print("command handler returned error code ");
    Serial.println(rc);
    iostack_send_error(&request, rc);
  }
}

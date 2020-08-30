#include <SPI.h>
extern SPIClass SPI_2;

#include "w5500.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define swap16(x) (((uint16_t) x >> 8) + ((uint16_t) x << 8))


// Make sure helper functions (see end of file) are always inlined
uint8_t w55_exchange(uint8_t x) __attribute__((always_inline));
void w55_transmit(uint8_t x) __attribute__((always_inline));
void w55_enable(void) __attribute__((always_inline));
void w55_disable(void) __attribute__((always_inline));


void w55_write(uint16_t addr, uint8_t block, uint8_t data)
{
  w55_enable();
  w55_transmit(addr >> 8);
  w55_transmit(addr);
  w55_transmit((block << 3) | W5500_RWB);
  w55_transmit(data);
  w55_disable();
}


void w55_write16(uint16_t addr, uint8_t block, uint16_t data)
{
  w55_enable();
  w55_transmit(addr >> 8);
  w55_transmit(addr);
  w55_transmit((block << 3) | W5500_RWB);
  w55_transmit(data >> 8);
  w55_transmit(data);
  w55_disable();
}


void w55_writen(uint16_t addr, uint8_t block, uint8_t *src, uint16_t size)
{
  w55_enable();
  w55_transmit(addr >> 8);
  w55_transmit(addr);
  w55_transmit((block << 3) | W5500_RWB);

  while (size-- != 0)
    w55_transmit(*src++);

  w55_disable();
}


uint8_t w55_read(uint16_t addr, uint8_t block)
{
  w55_enable();
  w55_transmit(addr >> 8);
  w55_transmit(addr);
  w55_transmit(block << 3);
  uint8_t value = w55_exchange(0x00);
  w55_disable();

  return value;
}


uint16_t w55_read16(uint16_t addr, uint8_t block)
{
  // Read multiple times until two consecutive reads have the same result
  w55_enable();
  w55_transmit(addr >> 8);
  w55_transmit(addr);
  w55_transmit(block << 3);
  uint16_t new_value = (w55_exchange(0x00) << 8) + w55_exchange(0x00);
  w55_disable();

  // TODO: Add timeout
  uint16_t value;
  do {
    value = new_value;

    w55_enable();
    w55_transmit(addr >> 8);
    w55_transmit(addr);
    w55_transmit(block << 3);
    new_value = (w55_exchange(0x00) << 8) + w55_exchange(0x00);
    w55_disable();
  } while (new_value != value);

  return value;
}


void w55_readn(uint16_t addr, uint8_t block, uint8_t *dest, uint16_t size)
{
  w55_enable();
  w55_transmit(addr >> 8);
  w55_transmit(addr);
  w55_transmit(block << 3);
  while (size-- != 0) {
    *dest++ = w55_exchange(0x00);
  }
  w55_disable();
}


void w55_init(void)
{
  // Initialise SPI
//  PORTB = PORTB | (1 << PB0) | (1 << PB6);
//  DDRB = DDRB | (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB6);
//  SPCR = (1 << SPE) | (1 << MSTR);  // SPI master mode 0
//  SPSR |= (1 << SPI2X);             // SPI clock rate clock / 2 = 8 MHz

  w55_write(W5500_MR, W5500_BLB_COM, W5500_MR_SOFTRST | (1 << 1));

  // TODO: Add timeout
  while (w55_read(W5500_MR, W5500_BLB_COM) & W5500_MR_SOFTRST)
    ;
}


uint8_t w55_config(struct w5500_config *cfg)
{
  // Set gateway, MAC, subnet mask, and IP
  w55_writen(W5500_GAR, W5500_BLB_COM, (uint8_t *) cfg,
             sizeof(struct w5500_config));

  // Read back and compare
  struct w5500_config chk;
  w55_readn(W5500_GAR, W5500_BLB_COM, (uint8_t *) &chk,
            sizeof(struct w5500_config));
  for (uint8_t i = 0; i != sizeof(struct w5500_config); i++)
    if (((uint8_t *) cfg)[i] != ((uint8_t *) &chk)[i])
      return 1;

  return 0;
}


uint8_t w55_next_free_socket(void)
{
  uint8_t socket = 0;
  while (socket < W5500_NUM_SOCKETS) {
    uint8_t socket_status =
        w55_read(W5500_SR_OFFSET, W5500_BLB_SKT_REG(socket));
    if (socket_status == W5500_SKT_SR_CLOSED)
      break;

    socket++;
  }

  return socket;
}


void w55_command(uint8_t socket, uint8_t cmd)
{
  if (socket >= W5500_NUM_SOCKETS)
    return;

  uint8_t block = W5500_BLB_SKT_REG(socket);
  w55_write(W5500_CR_OFFSET, block, cmd);

  // Wait for command to be accepted (note: doesn't mean it completed)
  // TODO: Add timeout
  while (w55_read(W5500_CR_OFFSET, block))
    ;
}


uint8_t w55_udp_open(uint16_t port)
{
  uint8_t socket = w55_next_free_socket();
  if (socket >= W5500_NUM_SOCKETS)
    return W5500_NUM_SOCKETS;

  // Close socket
  w55_command(socket, W5500_SKT_CR_CLOSE);

  // Set to UDP mode
  uint8_t block = W5500_BLB_SKT_REG(socket);
  w55_write(W5500_MR_OFFSET, block, W5500_SKT_SR_UDP);

  // Set port
  w55_write16(W5500_PORT_OFFSET, block, port);

  // Open and check
  w55_command(socket, W5500_SKT_CR_OPEN);
  if (w55_read(W5500_SR_OFFSET, block) != W5500_SKT_SR_UDP) {
    w55_command(socket, W5500_SKT_CR_CLOSE);
    return W5500_NUM_SOCKETS;
  }

  return socket;
}


uint16_t w55_udp_read(uint16_t socket, struct w5500_udp_header *header,
                      uint8_t *dst, uint16_t size)
{
  if (socket >= W5500_NUM_SOCKETS)
    return 0;

  if (!dst)
    size = 0;

  const uint8_t block = W5500_BLB_SKT_REG(socket);
  const uint8_t rx_block = W5500_BLB_SKT_RX(socket);
  const uint8_t header_size = 8;

  uint16_t get_size = w55_read16(W5500_RX_RSR_OFFSET, block);
  if (get_size == 0 || get_size < header_size)
    // FIXME: Handle size < 8 differently: skip incomplete packet
    return 0;

  get_size -= header_size;

  uint16_t rxrd = w55_read16(W5500_RX_RD_OFFSET, block);

  // Read header
  w55_readn(rxrd, rx_block, (uint8_t *) header, header_size);
  rxrd += header_size;

  // Swap endianess of port and size
  header->port = swap16(header->port);
  header->size = swap16(header->size);

  w55_readn(rxrd, rx_block, dst, MIN(header->size, size));
  rxrd += header->size;

  w55_write16(W5500_RX_RD_OFFSET, block, rxrd);
  w55_command(socket, W5500_SKT_CR_RECV);

  return header->size;
}


static uint16_t get_free_size[W5500_NUM_SOCKETS] = {
    0,
};
static uint16_t txwr[W5500_NUM_SOCKETS] = {
    0,
};


uint8_t w55_udp_begin(uint8_t socket, struct w5500_udp_header *header)
{
  if (socket >= W5500_NUM_SOCKETS)
    return 1;

  uint8_t block = W5500_BLB_SKT_REG(socket);

  // Write destination IP and port
  header->port = swap16(header->port);
  w55_writen(W5500_DIPR_OFFSET, block, (uint8_t *) header, 6);

  // Restore endianess if client wants to reuse port afterwards
  header->port = swap16(header->port);

  get_free_size[socket] = w55_read16(W5500_TX_FSR_OFFSET, block);
  txwr[socket] = w55_read16(W5500_TX_WR_OFFSET, block);

  return 0;
}


uint16_t w55_udp_write(uint8_t socket, uint8_t *src, uint16_t size)
{
  if (size == 0 || !src || socket >= W5500_NUM_SOCKETS)
    return 0;

  if (get_free_size[socket] < size) {
    get_free_size[socket] = 0;  // Stop writing if one part failed
    return 0;
  }

  const uint8_t tx_block = W5500_BLB_SKT_TX(socket);
  w55_writen(txwr[socket], tx_block, src, size);

  txwr[socket] += size;
  get_free_size[socket] -= size;

  return size;
}


uint8_t w55_udp_end(uint8_t socket)
{
  if (socket >= W5500_NUM_SOCKETS)
    return 1;

  uint8_t block = W5500_BLB_SKT_REG(socket);

  w55_write16(W5500_TX_WR_OFFSET, block, txwr[socket]);
  w55_command(socket, W5500_SKT_CR_SEND);

  // Set to safe value in case user calls w55_udp_end() several times
  get_free_size[socket] = 0;

  // TODO: Add timeout
  while (1) {
    uint8_t ir = w55_read(W5500_IR_OFFSET, block);

    if (ir & W5500_IR_SEND_OK) {
      w55_write(W5500_IR_OFFSET, block, W5500_IR_SEND_OK | W5500_IR_TIMEOUT);
      return 0;
    } else if (ir & W5500_IR_TIMEOUT) {
      w55_write(W5500_IR_OFFSET, block, W5500_IR_SEND_OK | W5500_IR_TIMEOUT);
      return 1;
    }
  }

  // Timed out
  return 1;
}


// Helper functions
uint8_t w55_exchange(uint8_t x)
{
  uint8_t result = SPI_2.transfer(x);
  return result;
}


void w55_transmit(uint8_t x)
{
  SPI_2.transfer(x);
}


void w55_enable(void)
{
//  W55_CS_PORT &= ~(1 << W55_CS_BIT);
  digitalWrite(SS,LOW);
  //SPI_2.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
}


void w55_disable(void)
{
//  W55_CS_PORT |= (1 << W55_CS_BIT);
  digitalWrite(SS,HIGH);
  //SPI_2.endTransaction();
}

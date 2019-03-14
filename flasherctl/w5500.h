#ifndef __W5500_H__
#define __W5500_H__

// w5500 header

// See:
// https://learn.adafruit.com/adafruit-feather-m0-basic-proto/adapting-sketches-to-m0#aligned-memory-access-7-13
// http://forum.arduino.cc/index.php?topic=184916.0
// for an explanation of struct __attribute__((packed)) 

#include <Arduino.h>

// Chip select configuration
#define W55_CS_PIN SS

// Max. number of sockets
#define W5500_NUM_SOCKETS 8

// Read/write bit
#define W5500_RWB (1 << 2)

// Common base register block and macros to calculate a socket's base register blocks
#define W5500_BLB_COM        0x00
#define W5500_BLB_SKT_REG(n) (0x01 + (n * 0x04))
#define W5500_BLB_SKT_TX(n)  (0x02 + (n * 0x04))
#define W5500_BLB_SKT_RX(n)  (0x03 + (n * 0x04))

// Common register addresses
#define  W5500_MR   0x0000  // Mode register
#define  W5500_GAR  0x0001  // Gateway address (4 bytes)
#define  W5500_SUBR 0x0005  // Subnet mask address (4 bytes)
#define  W5500_SHAR 0x0009  // Source hardware address (MAC, 6 bytes)
#define  W5500_SIPR 0x000f  // Source IP address (4 bytes)

// Interrupt register bits
#define W5500_IR_SEND_OK (1 << 4)
#define W5500_IR_TIMEOUT (1 << 3)
#define W5500_IR_RECV    (1 << 2)
#define W5500_IR_DISCON  (1 << 1)
#define W5500_IR_CON     (1 << 0)

// Offsets in a socket's base register block (W5500_BLB_SKT_REG(n)).
#define  W5500_MR_OFFSET     0x0000  // Socket mode
#define  W5500_CR_OFFSET     0x0001  // Socket command
#define  W5500_IR_OFFSET     0x0002  // Socket interrupt
#define  W5500_SR_OFFSET     0x0003  // Socket status
#define  W5500_PORT_OFFSET   0x0004  // Socket port (2 bytes)
#define  W5500_DIPR_OFFSET   0x000c  // Socket dest. IP address (4 bytes)
#define  W5500_DPORT_OFFSET  0x0010  // Socket dest. port (2 bytes)
#define  W5500_TX_FSR_OFFSET 0x0020  // Socket transmit free size (2 bytes)
#define  W5500_TX_WR_OFFSET  0x0024  // Socket transmit write pointer (2 bytes)
#define  W5500_RX_RSR_OFFSET 0x0026  // Socket receive received size (2 bytes)
#define  W5500_RX_RD_OFFSET  0x0028  // Socket receive read pointer (2 bytes)

// Chip mode register bits
#define  W5500_MR_SOFTRST (1 << 7)  // Soft reset

// Socket status register states
#define  W5500_SKT_SR_CLOSED      0x00  // Closed
#define  W5500_SKT_SR_INIT        0x13  // Initialising
#define  W5500_SKT_SR_LISTEN      0x14  // Listening
#define  W5500_SKT_SR_ESTABLISHED 0x17  // Connected
#define  W5500_SKT_SR_CLOSE_WAIT  0x1c  // Closing
#define  W5500_SKT_SR_UDP         0x22  // UDP socket

// Socket command register values
#define  W5500_SKT_CR_OPEN  0x01  // Open socket
#define  W5500_SKT_CR_CLOSE 0x10  // Mark as closed
#define  W5500_SKT_CR_SEND  0x20  // Transmit data from TX buffer
#define  W5500_SKT_CR_RECV  0x40  // Receive data into RX buffer

// Socket mode register values
#define  W5500_SKT_MR_CLOSE 0x00  // Unused socket
#define  W5500_SKT_MR_UDP   0x02  // UDP


struct __attribute__((packed)) w5500_config {
  uint8_t gateway_addr[4];
  uint8_t subnet_mask[4];
  uint8_t mac_address[6];
  uint8_t ip_address[4];
};

struct __attribute__((packed)) w5500_udp_header {
  uint8_t ip_address[4];
  uint16_t port;
  uint16_t size;
};


void w55_init(void);

void w55_write(uint16_t addr, uint8_t block, uint8_t data);
void w55_writen(uint16_t addr, uint8_t block, uint8_t *src, uint16_t size);
uint8_t w55_read(uint16_t addr, uint8_t block);
void w55_readn(uint16_t addr, uint8_t block, uint8_t dest[], uint16_t size);

uint8_t w55_config(struct w5500_config *cfg);

uint8_t w55_udp_open(uint16_t port);
uint16_t w55_udp_read(uint16_t socket, struct w5500_udp_header *header,
                      uint8_t *dst, uint16_t size);
uint8_t w55_udp_begin(uint8_t socket, struct w5500_udp_header *header);
uint16_t w55_udp_write(uint8_t socket, uint8_t *src, uint16_t size);
uint8_t w55_udp_end(uint8_t socket);

#endif

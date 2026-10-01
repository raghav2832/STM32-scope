#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>

/*
 * CRC-16/CCITT-FALSE
 * Polynomial: 0x1021, Init: 0xFFFF, no reflection
 * Standard used in XMODEM, many embedded protocols
 */
uint16_t crc16_update(uint16_t crc, uint8_t byte);
uint16_t crc16_buf(const uint8_t *buf, uint16_t len);

#endif

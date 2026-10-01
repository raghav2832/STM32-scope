#include "crc16.h"

uint16_t crc16_update(uint16_t crc, uint8_t byte)
{
    crc ^= ((uint16_t)byte << 8);
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x8000U)
            crc = (crc << 1) ^ 0x1021U;
        else
            crc <<= 1;
    }
    return crc;
}

uint16_t crc16_buf(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0; i < len; i++)
        crc = crc16_update(crc, buf[i]);
    return crc;
}

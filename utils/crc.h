#ifndef SIMPLE_CRC_H
#define SIMPLE_CRC_H

#include <inttypes.h>
#include <stddef.h>

inline uint16_t crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }
  return crc;
}

#endif // SIMPLE_CRC_H
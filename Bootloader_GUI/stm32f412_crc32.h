/*
 * stm32f412_crc32.h
 *  Created on: 12 Jan, 2023
 *  Author: abdo daood < abdo454daood@gmail.com >
 */

#ifndef STM32F412_CRC32_H
#define STM32F412_CRC32_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

class STM32F412_CRC32 {
public:
  STM32F412_CRC32();
  ~STM32F412_CRC32();
  uint32_t CalcCRC(uint8_t *pData, uint32_t DataLength);
  uint32_t CalcCRC_Add(uint32_t Checksum_old, uint8_t *pData,
                       uint32_t DataLength);

private:
  uint8_t reverse_char_bits(uint8_t);
  uint32_t reverse_integer_bits(uint32_t);
};

#endif // STM32F412_CRC32_H

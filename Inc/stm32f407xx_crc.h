#ifndef CRC16_H
#define CRC16_H

#include <stm32f407xx.h>

uint16_t CRC16_Calculate(const uint8_t *data, uint32_t length);

#endif
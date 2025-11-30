#ifndef EEPROM_24C08_H
#define EEPROM_24C08_H

#include "main.h"

void EEPROM_WriteByte(uint16_t addr, uint8_t data);
uint8_t EEPROM_ReadByte(uint16_t addr);

#endif

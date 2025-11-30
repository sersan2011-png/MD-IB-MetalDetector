#include "main.h"
#include <stdint.h>

void EEPROM_WriteByte(uint16_t addr, uint8_t data) {
    uint8_t buffer[2] = {(uint8_t)(addr >> 8), (uint8_t)addr, data};
    HAL_I2C_Master_Transmit(&hi2c1, 0xA0, buffer, 3, 100);
    HAL_Delay(5);
}

uint8_t EEPROM_ReadByte(uint16_t addr) {
    uint8_t addr_byte = (uint8_t)addr;
    uint8_t data;
    HAL_I2C_Master_Transmit(&hi2c1, 0xA0, &addr_byte, 1, 100);
    HAL_I2C_Master_Receive(&hi2c1, 0xA1, &data, 1, 100);
    return data;
}

#ifndef _I2C_H
#define _I2C_H

#include "Main.h"

#define I2C_COMM_OK (1)
#define I2C_ERROR_BUS_BUSY (2)
#define I2C_ERROR_NACK (3)

uint8_t I2C_read(uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t *data_array);
uint8_t I2C_write(uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t *data_array);

#endif

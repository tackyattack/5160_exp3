#ifndef _I2C_H
#define _I2C_H

#include "Main.h"

#define I2C_COMM_OK (1)
#define I2C_ERROR_BUS_BUSY (2)
#define I2C_ERROR_NACK (3)

#define I2C_COMM_WRITE (0)
#define I2C_COMM_READ  (1)

uint8_t I2C_comm(uint8_t comm_type, uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t *data_array);
uint8_t I2C_read(uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t *data_array);
#define I2C_write(device_addr, int_addr, int_addr_sz, num_bytes, data) I2C_comm(I2C_COMM_WRITE, device_addr, int_addr, int_addr_sz, num_bytes, data)
#endif

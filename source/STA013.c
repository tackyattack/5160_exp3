#include "STA013.h"
#include "PORT.h"
#include "Main.h"
#include "I2C.h"
#include "outputs.h"
#include <stdio.h>

#define STA013_DEVICE_ADDR (0x43)
#define CFG_TABLE_SZ (18)

#define TEST_FAIL (0)
#define TEST_PASS (1)

// row 1: register addresses
// row 2: register values
uint8_t code cfg_table[2][CFG_TABLE_SZ] =
{0x54, 0x55, 6, 11,  82,  81, 101, 100, 80, 97, 0x05, 0x0d, 0x18, 0x0c, 0x46, 0x48, 0x7d, 0x72,
0x07, 0x10,  9,  2, 184,   0,   0,   0,  6,  5, 0xa1, 0x00, 0x04, 0x05, 0x07, 0x07, 0x07, 0x01};

uint8_t STA013_test_read(uint8_t *ID_reg)
{
  uint8_t err;
  uint8_t timeout = 50;
  uint8_t rcv[1];
  do
  {
    // check internal address 1
    write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, SET_BIT);
    err = I2C_read(STA013_DEVICE_ADDR, 0x01, 1, 1, rcv);
    write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, CLEAR_BIT);
    timeout--;
  } while((err != I2C_COMM_OK) && (timeout!=0));
  if(timeout==0)
  {
    printf("Error: timeout while testing read\n");
    return TEST_FAIL;
  }
  *ID_reg = rcv[0];
  if(rcv[0] != 0xAC)
  {
    printf("Error: incorrect ID register while testing read\n");
    return TEST_FAIL;
  }

  printf("Read test passed: ID register: <%2.2bX>\n", *ID_reg);
  return TEST_PASS;
}

uint8_t STA013_test_write()
{
  uint8_t err;
  uint8_t timeout = 50;
  uint8_t send[1];
  uint8_t ID_reg;
  send[0] = 0xBE;
  do
  {
    // check internal address 1
    write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, SET_BIT);
    err = I2C_write(STA013_DEVICE_ADDR, 0x01, 1, 1, send);
    write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, CLEAR_BIT);
    timeout--;
  } while((err != I2C_COMM_OK) && (timeout!=0));
  if(timeout==0)
  {
    printf("Error: timeout while testing write\n");
    return TEST_FAIL;
  }

  if(STA013_test_read(&ID_reg) != TEST_PASS) return TEST_FAIL;

  if(ID_reg != 0xBE)
  {
    printf("Error: incorrect ID register while testing write\n");
    return TEST_FAIL;
  }

  printf("Write test passed: ID register: <%2.2bX>\n", ID_reg);
  return TEST_PASS;
}

uint8_t STA013_test()
{
  uint8_t ID_reg;
  if(STA013_test_read(&ID_reg) != TEST_PASS) return TEST_FAIL;
  if(STA013_test_write() != TEST_PASS) return TEST_FAIL;
}

uint8_t STA013_send_cfg_file(uint8_t *cfg_p)
{
  uint8_t timeout, status;
  uint16_t index;
  uint32_t internal_addr;
  uint8_t send_array[1];

  internal_addr = 0x00;
  index=0;
  // write config file
  while((internal_addr != 0xFF)&&(timeout!=0))
  {
    timeout=50;
    do
    {
      write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, SET_BIT);
      status = I2C_write(STA013_DEVICE_ADDR, internal_addr, 1, 1, send_array);
      write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, CLEAR_BIT);
    }while((status!=I2C_COMM_OK)&&(timeout!=0));
    if(timeout==0)
    {
      return STA013_CFG_ERROR;
      printf("STA013 config timeout error\n");
    }
    internal_addr = cfg_p[index];
    index++;
    send_array[0] = cfg_p[index];
    index++;
  }

  return STA013_CFG_SENT;

}

uint8_t STA013_init()
{
  uint8_t timeout, status;
  uint16_t index;
  uint32_t internal_addr;
  uint8_t send_array[1];
  extern uint8_t code CONFIG;
  extern uint8_t code CONFIG2;
  index=0;
  timeout=50;

  write_port_bit(STA013_RESET_PORT, STA013_RESET_PIN, CLEAR_BIT);
  write_port_bit(STA013_RESET_PORT, STA013_RESET_PIN, SET_BIT);

  // test read and write I2C functions with the STA013
  if(STA013_test() != TEST_PASS)
  {
    printf("STA013 init error\n");
    return STA013_INIT_ERROR;
  }

  if(STA013_send_cfg_file(&CONFIG)!=STA013_CFG_SENT) return STA013_INIT_ERROR;
  if(STA013_send_cfg_file(&CONFIG2)!=STA013_CFG_SENT) return STA013_INIT_ERROR;

  for(index = 0; index < CFG_TABLE_SZ; index++)
  {
    timeout=50;
    internal_addr = cfg_table[0][index];
    send_array[0] = cfg_table[1][index];
    do
    {
      write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, SET_BIT);
      status = I2C_write(STA013_DEVICE_ADDR, internal_addr, 1, 1, send_array);
      write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, CLEAR_BIT);
      timeout--;
    }while((status!=I2C_COMM_OK)&&(timeout!=0));
    if(timeout == 0)
    {
      printf("STA013 init error while setting config table\n");
      return STA013_INIT_ERROR;
    }
  }

  return STA013_INIT_OK;
}

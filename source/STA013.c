#include "STA013.h"
#include "PORT.h"
#include "Main.h"
#include "I2C.h"
#include "outputs.h"
#include <stdio.h>

#define STA013_DEVICE_ADDR (0x43)
#define CFG_TABLE_SZ (17)

#define TEST_FAIL (0)
#define TEST_PASS (1)

// row 1: register addresses
// row 2: register values
uint8_t code cfg_table[2][CFG_TABLE_SZ] =
{0x54, 0x55, 6, 11,  97,  80, 101,  82, 100, 81,   5, 0x0d, 0x18, 0x0c, 0x46, 0x48, 0x72,
 0x07, 0x10, 9,  2,   5,   6,   0, 184,   0,  0, 161, 0x00, 0x04, 0x05, 0x07, 0x07, 0x01};
// 0x54 = 0x07: 16-bit output with 512 x oversampling
// 0x55 = 0x10: MSB first, right padded, I2S format, 16-bit output
// Next 9 registers: 14.7456MHz with 512 oversampling ratio
// 0x0d = 0x00: the data are sent with the falling edge of SCKR and sampled on the rising edge
// 0x18 = 0x04: Enable the Data Request Signal for multimedia mode
// 0x0C = 0x05: Set the Data Request Polarity to active low
// 0x46 = 0x07: left attenuation set to 7dB
// 0x48 = 0x07: right attenuation set to 7dB
// 0x72 = 0x01: start decoding

uint8_t STA013_test_read_ID(uint8_t *ID_reg)
{
  uint8_t err;
  uint8_t timeout = 50;
  uint8_t rcv[1];
  do
  {
    // check internal address 1
    err = I2C_read(STA013_DEVICE_ADDR, 0x01, 1, 1, rcv);
    timeout--;
  } while((err != I2C_COMM_OK) && (timeout!=0));

  if(timeout==0)
  {
    printf("Error: timeout while testing read\n");
    return TEST_FAIL;
  }
  if(err != I2C_COMM_OK)
  {
    printf("Error: I2C while testing read: %d\n", err);
    return TEST_FAIL;
  }
  *ID_reg = rcv[0];
  if(rcv[0] != 0xAC)
  {
    printf("Error: incorrect ID register while testing read");
    printf(" ID register: <0x%2.2bX>\n", ID_reg);
    return TEST_FAIL;
  }
  return TEST_PASS;
}

uint8_t STA013_send_cfg_file(uint8_t *cfg_p)
{
  uint8_t timeout, status;
  uint16_t index;
  uint32_t internal_addr;
  uint8_t send_array[1];
  index=0;
  internal_addr = cfg_p[index];
  index++;
  send_array[0] = cfg_p[index];
  index++;
  // write config file
  while(internal_addr != 0xFF)
  {
    timeout=50;
    // check to make sure address isn't end of array
    do
    {
      status = I2C_write(STA013_DEVICE_ADDR, internal_addr, 1, 1, send_array);
      timeout--;
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

uint8_t STA013_send_cfg_table()
{
  uint8_t recv[1], send[1], timeout, status;
  uint16_t index;
  uint32_t internal_addr;
  for(index = 0; index < CFG_TABLE_SZ; index++)
  {
    timeout=50;
    internal_addr = cfg_table[0][index];
    send[0] = cfg_table[1][index];
    // set register
    do
    {
      status = I2C_write(STA013_DEVICE_ADDR, internal_addr, 1, 1, send);
      timeout--;
    }while((status!=I2C_COMM_OK)&&(timeout!=0));
    if(timeout == 0)
    {
      printf("STA013 init error while setting config table: timeout\n");
      return STA013_INIT_ERROR;
    }
    if(status != I2C_COMM_OK)
    {
      printf("STA013 init error while setting config table: communication\n");
      return STA013_INIT_ERROR;
    }

    // read back to verify
    timeout = 0;
    recv[0] = 0x00;
    do
    {
      status = I2C_read(STA013_DEVICE_ADDR, internal_addr, 1, 1, recv);
      timeout--;
    }while((status!=I2C_COMM_OK)&&(timeout!=0));
    if(timeout == 0)
    {
      printf("STA013 init error while verifying config table: timeout\n");
      return STA013_INIT_ERROR;
    }
    if(status != I2C_COMM_OK)
    {
      printf("STA013 init error while setting config table: communication\n");
      return STA013_INIT_ERROR;
    }
    if(recv[0] != send[0])
    {
      printf("STA013 init error while setting config table: register values do not match\n");
      printf("Should be: <0x%2.2bX>  instead got: <0x%2.2bX>\n", send[0], recv[0]);
      return STA013_INIT_ERROR;
    }
    else
    {
      printf("Set register: <0x%2.2bX> to <0x%2.2bX>  verified as: <0x%2.2bX>\n", (uint8_t)internal_addr, send[0],recv[0]);
    }
  }
  return STA013_CFG_SENT;
}

uint8_t STA013_init()
{
  uint8_t ID_reg;
  extern uint8_t code CONFIG;
  extern uint8_t code CONFIG2;

  write_port_bit(STA013_RESET_PORT, STA013_RESET_PIN, CLEAR_BIT);
  write_port_bit(STA013_RESET_PORT, STA013_RESET_PIN, SET_BIT);

  // test read and write I2C functions with the STA013
  write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, CLEAR_BIT);
  if(STA013_test_read_ID(&ID_reg) != TEST_PASS)
  {
    printf("STA013 init error\n");
    return STA013_INIT_ERROR;
  }
  write_port_bit(STA013_TRIGGER_PORT, STA013_TRIGGER_PIN, SET_BIT);
  printf("Read test passed: ID register: <0x%2.2bX>\n", ID_reg);

  if(STA013_send_cfg_file(&CONFIG)!=STA013_CFG_SENT) return STA013_INIT_ERROR;
  if(STA013_send_cfg_file(&CONFIG2)!=STA013_CFG_SENT) return STA013_INIT_ERROR;

  if(STA013_send_cfg_table() != STA013_CFG_SENT) return STA013_INIT_ERROR;

  return STA013_INIT_OK;
}

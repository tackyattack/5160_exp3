#include "I2C.h"
#include "PORT.h"
#include "Main.h"
#include <stdio.h>

#define CLOCK_CONTINUE (0)
#define CLOCK_STOP (1)
#define I2C_FREQ (25000UL)
#define I2C_RELOAD ((uint16_t)(65536-((OSC_FREQ)/(OSC_PER_INST*I2C_FREQ*2UL))))
#define I2C_RELOAD_H (I2C_RELOAD/256UL)
#define I2C_RELOAD_L (I2C_RELOAD%256UL)


void I2C_clock_start(void)
{
  //16b timer mode
  TMOD &= 0x0F;
  TMOD |= 0x10;
  ET1 = 0;
  TH1 = I2C_RELOAD_H;
  TL1 = I2C_RELOAD_L;
  TF1 = 0;
  TR1 = 1;
}

void I2C_clock_delay(uint8_t control)
{
  if(TR1 == 1)
  {
    while(TF1 == 0);
  }

  TR1 = 0;
  if(control == CLOCK_CONTINUE)
  {
    TH1 = I2C_RELOAD_H;
    TL1 = I2C_RELOAD_L;
    TF1 = 0;
    TR1 = 1;
  }
}

uint8_t get_ACK()
{
  uint8_t sent_bit;
  // Get the ACK
  I2C_clock_delay(CLOCK_CONTINUE);
  SCL=0;
  SDA=1; // make input
  I2C_clock_delay(CLOCK_CONTINUE);
  SCL=1;
  while(SCL!=1);
  // verify ACK
  sent_bit = SDA;
  if(sent_bit != 0) return I2C_ERROR_NACK;
  return I2C_COMM_OK;
}

void send_ACK()
{
  I2C_clock_delay(CLOCK_CONTINUE);
  SCL=0;
  SDA=0;
  I2C_clock_delay(CLOCK_CONTINUE);
  SCL=1;
  while(SCL!=1);
}

void send_NACK()
{
  I2C_clock_delay(CLOCK_CONTINUE);
  SCL=0;
  SDA=1;
  I2C_clock_delay(CLOCK_CONTINUE);
  SCL=1;
  while(SCL!=1);
}

void send_STOP()
{
  I2C_clock_delay(CLOCK_CONTINUE);
  SCL=0;
  SDA=0;
  I2C_clock_delay(CLOCK_CONTINUE);
  SCL=1;
  while(SCL!=1);
  SDA=1;
}

uint8_t send_byte(uint8_t send_val)
{
  uint8_t num_bits, send_bit, sent_bit;
  num_bits = 8;
  do
  {
    I2C_clock_delay(CLOCK_CONTINUE);
    SCL=0;
    num_bits--;
    send_bit=((send_val>>num_bits)&0x01); // shift to get next bit and mask off
    SDA=(bit)send_bit;

    I2C_clock_delay(CLOCK_CONTINUE);
    SCL=1;
    while(SCL!=1); // wait for SCL to be '1'
    // verify signal matches what was written to SDA
    sent_bit=SDA; // read
    if(sent_bit != send_bit) return I2C_ERROR_BUS_BUSY;
  }while(num_bits>0);
  return get_ACK();
}

uint8_t I2C_write(uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t *data_array)
{

  uint8_t return_val, send_val;
  uint16_t index;
  SDA=1;
  SCL=1;
  // device addr + int addr + data
  num_bytes = num_bytes + int_addr_sz;
  if((SCL!=1) || (SDA!=1)) return I2C_ERROR_BUS_BUSY;
  I2C_clock_start();
  send_val = device_addr<<1;
  send_val &= 0xFE; // clear LSB (R/W bit)
  // create start condition
  SDA=0;

  // send device addr
  return_val = send_byte(send_val);
  if(return_val != I2C_COMM_OK)
  {
      I2C_clock_delay(CLOCK_STOP);
      return return_val;
  }

  for(index = 0; index < num_bytes; index++)
  {
    if(index < int_addr_sz)
    {
      send_val = (int_addr >> (8*index))&0xFF;
    }
    else
    {
      send_val = data_array[index-int_addr_sz];
    }
    // Send and get ACK
    return_val = send_byte(send_val);
    if(return_val != I2C_COMM_OK)
    {
        I2C_clock_delay(CLOCK_STOP);
        return return_val;
    }
  }
  send_STOP();
  I2C_clock_delay(CLOCK_STOP);
  return I2C_COMM_OK;

}

uint8_t I2C_read(uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t *data_array)
{

  uint8_t return_val, send_val, rcv_val, rcv_bit, num_bits;
  uint16_t index;
  return_val = I2C_COMM_OK;
  I2C_write(device_addr, int_addr, int_addr_sz, 0, data_array);

  SDA=1;
  SCL=1;
  if((SCL!=1) || (SDA!=1)) return I2C_ERROR_BUS_BUSY;
  I2C_clock_start();

  send_val = device_addr<<1;
  send_val |= 0x01; // set LSB (R/W bit)

  // create start condition
  SDA=0;

  // send device address
  return_val = send_byte(send_val);
  if(return_val != I2C_COMM_OK)
  {
    I2C_clock_delay(CLOCK_STOP);
    return return_val;
  }

  // send bytes
  for(index = 0; index < num_bytes; index++)
  {
    num_bits = 8;
    rcv_val = 0;
    while(num_bits > 0)
    {
      num_bits--;
      I2C_clock_delay(CLOCK_CONTINUE);
      SCL=0;
      SDA=1;
      I2C_clock_delay(CLOCK_CONTINUE);
      SCL=1;
      while(SCL!=1);
      rcv_bit = SDA;
      rcv_val |= (rcv_bit)<<num_bits;
    }
    data_array[index] = rcv_val;
    if(index == num_bytes - 1)
    {
      send_NACK(); // no more bytes
    }
    else
    {
      send_ACK(); // more bytes
    }
  }

  send_STOP();
  I2C_clock_delay(CLOCK_STOP);
  return I2C_COMM_OK;
}

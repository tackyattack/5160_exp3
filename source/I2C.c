#include "I2C.h"
#include "PORT.h"
#include "Main.h"

#define CLOCK_CONTINUE (0)
#define CLOCK_STOP (1)
#define I2C_FREQ (25000)
#define I2C_RELOAD (65536 - ((OSC_FREQ)/(I2C_FREQ*OSC_PER_INST*2UL)))
#define I2C_RELOAD_H (I2C_RELOAD/256)
#define I2C_RELOAD_L (I2C_RELOAD%256)


void I2C_clock_start(void)
{
  //16b timer mode
  TMOD &= 0x0F;
  TMOD |= 0x10;
  ET1 = 0;
  TH1 = I2C_RELOAD_H;
  TL1 = I2C_RELOAD_L;
  TF1 = 0;
  TR1 = 0;
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
    TR1 = 0;
  }
}

uint8_t I2C_comm(uint8_t comm_type, uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t *data_array)
{
  uint8_t return_val, send_val, rcv_val, num_bits, sent_bit, send_bit;
  uint16_t index;
  return_val = I2C_COMM_OK;
  int_addr_sz = int_addr_sz*8; // convert bytes to bits
  SDA=1;
  SCL=1;
  num_bytes++; // include device address
  if((SCL==1) && (SDA==1))
  {
    I2C_clock_start();
    send_val = device_addr<<1;
    if(comm_type == I2C_COMM_READ) send_val |= 0x01; // set LSB (R/W bit)
    // send start condition
    SDA=0;
    index=0;
    // byte loop
    do
    {
      // bit loop
      num_bits = 8;
      do
      {
        I2C_clock_delay(CLOCK_CONTINUE);
        SCL=0;
        num_bits--;
        if(comm_type == I2C_COMM_READ)
        {
          SDA=1; // set to '1' for input
          rcv_val = rcv_val<<1;
        }
        else
        {
          send_bit=((send_val>>num_bits)&0x01); // shift to get next bit and mask off
          SDA=(bit)send_bit;
        }
        I2C_clock_delay(CLOCK_CONTINUE);
        SCL=1;
        while(SCL!=1); // wait for SCL to be '1'
        sent_bit=SDA; // read
        if(comm_type == I2C_COMM_READ)
        {
          rcv_val |= sent_bit;
        }
        else
        {
          if(sent_bit != send_bit) return_val = I2C_ERROR_BUS_BUSY;
        }
      }while((num_bits!=0)&&(return_val==I2C_COMM_OK));

      if(return_val == I2C_COMM_OK)
      {
        if(comm_type == I2C_COMM_READ)
        {
          // reading
          data_array[index] = rcv_val;
          index++;
          num_bytes--;
          if(num_bytes==0)
          {
            send_bit = 1; // NACK for last byte
          }
          else
          {
            send_bit = 0; // ACK for more bytes
          }
          I2C_clock_delay(CLOCK_CONTINUE);
          SCL=0;
          SDA=send_bit;
          I2C_clock_delay(CLOCK_CONTINUE);
          SCL=1;
          while(SCL!=1);
        }
        else
        {
          // writing
          I2C_clock_delay(CLOCK_CONTINUE);
          SCL=0;
          SDA=1; // set SDA high so slave can send ACK
          if(int_addr_sz!=0)
          {
            int_addr_sz = int_addr_sz - 8;
            send_val = (uint8_t)(int_addr>>int_addr_sz);
          }
          else
          {
            num_bytes--;
            send_val = data_array[index];
            index++;
          }

          I2C_clock_delay(CLOCK_CONTINUE);
          SCL=1;
          while(SCL!=1);
          sent_bit=SDA;
          if(send_bit!=0) return_val=I2C_ERROR_NACK;
        }
      }
    }while((num_bytes>0)&&(return_val==I2C_COMM_OK));

    // send stop condition
    if(return_val!=I2C_ERROR_BUS_BUSY)
    {
      I2C_clock_delay(CLOCK_CONTINUE);
      SCL=0;
      SDA=0;
      I2C_clock_delay(CLOCK_CONTINUE);
      SCL=1;
      while(SCL!=1);
      I2C_clock_delay(CLOCK_STOP);
      SDA=1;
    }
  }
  return I2C_COMM_OK;
}


uint8_t I2C_read(uint8_t device_addr, uint32_t int_addr, uint8_t int_addr_sz, uint16_t num_bytes, uint8_t *data_array)
{
  uint8_t return_val = I2C_COMM_OK;
  if(int_addr_sz != 0)
  {
    return_val = I2C_comm(I2C_COMM_WRITE, device_addr, int_addr, int_addr_sz, 0, data_array);
  }
  if(return_val != I2C_COMM_OK) return return_val;
  return I2C_comm(I2C_COMM_READ, device_addr, 0, 0, num_bytes, data_array);
}

#include <stdio.h>
#include "main.h"
#include "PORT.H"
#include "UART.h"
#include "Timer0_hardware_delay_1ms.h"
#include "LCD_routines.h"
#include "LED_Outputs.h"
#include "STA013.h"

uint8_t code exp3_string[]="STA013 Init";
uint8_t code exp3_string_success[]="STA013 Success";

main()
{
   uint8_t input_value;

   LEDS_ON(Red_LED);
   AUXR=0x0c;   // make all of XRAM available, ALE always on
   if(OSC_PER_INST==6)
   {
       CKCON0=0x01;  // set X2 clock mode
   }
   else if(OSC_PER_INST==12)
   {
       CKCON0=0x00;  // set standard clock mode
   }
   Timer0_DELAY_1ms(300);
   uart_init(9600);


   LCD_Init();
   LCD_Print(line1,0,exp3_string);
   if(STA013_init() == STA013_INIT_OK)
   {
     printf("STA013 init success\n");
     LCD_Print(line1,0,exp3_string_success);
     LEDS_ON(Green_LED);
   }
   else
   {
     printf("STA013 init failed\n");
     LEDS_ON(Amber_LED);
   }
   LEDS_OFF(Red_LED);

   while(1)
   {
	      input_value=UART_Receive();
        UART_Transmit(input_value);
   }
}

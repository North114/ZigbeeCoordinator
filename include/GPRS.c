#include "GPRS.h"

void Send_Data_to_GPRS(unsigned char id,unsigned int current,unsigned int voltage,unsigned char type,unsigned char ButtonStatus){
	USART0_Send_Byte(StartByte_Zigbee);//_delay_ms(10);
	USART0_Send_Byte(id);//_delay_ms(10);
	
	current = current * 100;
	USART0_Send_Byte(current / 256);
	USART0_Send_Byte(current % 256);//_delay_ms(10);
					
	voltage = voltage * 100;
	USART0_Send_Byte(voltage / 256);//_delay_ms(10);
	USART0_Send_Byte(voltage % 256);
					
	if(ButtonStatus == 0x01){
		USART0_Send_Byte(type);//type indicator
	} else if(ButtonStatus == 0x00) {
		USART0_Send_Byte(0x06);//type indicator
    }
	
	USART0_Send_Byte(COORDINATORID);//type indicator
	USART0_Send_Byte(EndByte_Zigbee);
}
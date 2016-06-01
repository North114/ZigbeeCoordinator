#ifndef GPRS_H

	#include "usart.h"

	#define GPRS_H
	#define COORDINATORID 1
	#define StartByte_Zigbee 0xAA
	#define EndByte_Zigbee 0x75 //End byte should less than 128,since it's a character
	
	void Send_Data_to_GPRS(unsigned char id,unsigned int current,unsigned int voltage,unsigned char type,unsigned char ButtonStatus);
	
#endif
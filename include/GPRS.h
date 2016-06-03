#ifndef GPRS_H
	
	#include "ZigBee.h"
	#include "usart.h"

	#define GPRS_H
	#define COORDINATORID 1
	
	#define Normal_RealTime_Data 0x50
	#define Last_RealTime_Data 0x80
	
	#define null 0
	
	void Send_Query_Command_to_ZigBee(unsigned char id);
	
	void Send_Cached_Data_to_GPRS(	unsigned char id,\
									unsigned int current,\
									unsigned int voltage,\
									unsigned char type,\
									unsigned char ButtonStatus);
									
	void Send_Received_Data_to_GPRS(volatile unsigned char* buf,\
									unsigned char len,\
									unsigned char id,\
									unsigned char type,\
									unsigned char ButtonStatus);
#endif
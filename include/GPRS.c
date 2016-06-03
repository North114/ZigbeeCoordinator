#include <assert.h>
#include <stdlib.h>
//#include <stdio.h>
#include "GPRS.h"
// for assert(expression)
#define __ASSERT_USE_STDERR
#undef NDEBUG
/**
 *  \brief Send Query Command to ZigBee Router
 *  
 *  \param [in] id Query Ruter Id
 *  \return void
 *  
 *  \details as brief
 */
void Send_Query_Command_to_ZigBee(unsigned char id) {
	/* Send Query Command to Routers */				
	USART1_Send_Byte(StartByte_Zigbee);
	//_delay_ms(1);
	USART1_Send_Byte(id);
	//_delay_ms(1);
	USART1_Send_Byte(ZigbeeQueryByte);//Command Byte
	//_delay_ms(1);
	USART1_Send_Byte(EndByte_Zigbee);
}
/**
 *  \brief Send Cached Data as Real-Time Data to GPRS end
 *  
 *  \param [in] id ZigBee Router ID
 *  \param [in] current Current Value
 *  \param [in] voltage Voltage Value
 *  \param [in] type Package Type
 *  \param [in] ButtonStatus Current Button Status
 *  \return void
 *  
 *  \details as brief
 */
void Send_Cached_Data_to_GPRS(unsigned char id,\
							unsigned int current,\
							unsigned int voltage,\
							unsigned char type,\
							unsigned char ButtonStatus) {
								
	USART0_Send_Byte(StartByte_Zigbee);//_delay_ms(10);
	USART0_Send_Byte(id);//_delay_ms(10);
	
	USART0_Send_Byte(current / 256);
	USART0_Send_Byte(current % 256);//_delay_ms(10);
					
	USART0_Send_Byte(voltage / 256);//_delay_ms(10);
	USART0_Send_Byte(voltage % 256);
					
	if(ButtonStatus == 0x01){
		USART0_Send_Byte(type);//type indicator in GPRS Mode
	} else if(ButtonStatus == 0x00) {
		USART0_Send_Byte(0x06);//type indicator in Bluetooth Mode
    }
	
	USART0_Send_Byte(COORDINATORID);//type indicator
	USART0_Send_Byte(EndByte_Zigbee);
}
 /**
 *  \brief Send ZigBee End Received Real-Time Data to GPRS end
 *  
 *  \param [in] buf buffer of ZigBee end Received Data
 *  \param [in] len ZigBee end Received Package Length
 *  \param [in] id ZigBee Router ID
 *  \param [in] type Package Type
 *  \param [in] ButtonStatus button status
 *  \return void
 *  
 *  \details as brief
 */
void Send_Received_Data_to_GPRS(volatile unsigned char* buf,\
								unsigned char len,\
								unsigned char id,\
								unsigned char type,\
								unsigned char ButtonStatus) {
	// the following line seems not works
	assert(len >= 6);
	
	if(buf != null) {
		USART0_Send_Byte(StartByte_Zigbee);//_delay_ms(10);
		USART0_Send_Byte(buf[0]);//_delay_ms(10);
		USART0_Send_Byte(buf[1]);
		USART0_Send_Byte(buf[2]);//_delay_ms(10);
		USART0_Send_Byte(buf[3]);//_delay_ms(10);
		USART0_Send_Byte(buf[4]);
		if (ButtonStatus == 0x01)
		{
			USART0_Send_Byte(type);//type indicator
		} else if(ButtonStatus == 0x00) {
			USART0_Send_Byte(buf[5]);
		}
		USART0_Send_Byte(COORDINATORID);
		USART0_Send_Byte(EndByte_Zigbee);
	} else {
		USART0_Send_Byte(StartByte_Zigbee);
		USART0_Send_Byte(id);//could be a random id
		USART0_Send_Byte(0x00);
		USART0_Send_Byte(0x00);
		USART0_Send_Byte(0x00);
		USART0_Send_Byte(0x00);
		USART0_Send_Byte(type);
		USART0_Send_Byte(COORDINATORID);
		USART0_Send_Byte(EndByte_Zigbee);
	}
}
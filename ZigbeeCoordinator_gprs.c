/*
CPU : ATmega644pA
Frequency : 16MHz

Function : Zigbee Coordinator(Version 01)

Added Function:Read Real-Time Data of all Router or Specified Router

Some Volatile need to be added before variable declare

changing log:

description:changing recent data meaning,now,recent data means 'today's' data;
date:01-26-2015

description:close global interrupt when we use twi;
date:01-27-2015

description:close global interrupt when we store received data into eeprom
date:03-17-2015
*/
#ifndef F_CPU
#define F_CPU 16000000UL  /* 16 MHz CPU clock */
#endif

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "include/init.h"
#include "include/usart.h"
#include "include/at24c128.h"
#include "include/ds1307.h"
#include "include/GPRS.h"
#include "include/ZigBee.h"

/* Coordinator Related Macro */
//#define DEBUG
//#define COORDINATORID 2 //defined in GPRS.h
#define CURRENT_THREASHOLE 2000
#define VOLTAGE_UPPER 24000
#define VOLTAGE_LOWER 20000

//#define RouterNum 80       //Total device number in a PAN

/* Bluetooth Related Macro */
#define recBufferSize_Bluetooth 10// larger than PackLen
#define StartByte_Bluetooth 0xBB
#define EndByte_Bluetooth 0x70 //end byte should less than 128,since it's a character
#define QUERYRETRYTIME 2

#define CACHE_TIME 250
#define CACHE_SPACE 200

#define MaxRouterNum 250
#define RETRY_MISSING_NODE //for query missing nodes
/* Function Declaration */

/* DS1307 Relatted Variable Defination */
unsigned char CurrentTime[7];

/* AT24C128 Relatted Variable Defination */
unsigned char W_EEprom_Array[BlockLength],R_EEprom_Array[BlockLength];
unsigned int FirstReadByteAddr = 0, LastReadByteAddr = 0;
unsigned int FirstUnReadByteAddr = 0,LastUnReadByteAddr = 0;
unsigned char EEpromFull;

/* Zigbee Relatted Variable Defination */
volatile unsigned char startFlag_Zigbee = 0;
volatile unsigned char recFlag_Zigbee = 0;//add a 'volatile' is very impportant
//we can also choose _Bool 
volatile unsigned char index_Zigbee = 0;
volatile unsigned char recNum_Zigbee = 0;
volatile unsigned char recBuffer_Zigbee[recBufferSize_Zigbee];

unsigned char ACK_Zigbee[Zigbee_AckLen] = {StartByte_Zigbee,0,0,0,EndByte_Zigbee};


/* Bluetooth Relatted Variable Defination */
volatile unsigned char startFlag_Bluetooth = 0,recFlag_Bluetooth;//add a 'volatile' is very impportant
volatile unsigned char index_Bluetooth = 0,recNum_Bluetooth = 0;
volatile unsigned char recBuffer_Bluetooth[recBufferSize_Bluetooth];
volatile unsigned char recData_Bluetooth[recBufferSize_Bluetooth];
volatile unsigned char BlockNum;
volatile unsigned char RealTimeQuery = 0;

/* data cacheing */
volatile unsigned char cache_current[CACHE_SPACE] = {0};
volatile unsigned int cache_voltage[CACHE_SPACE] = {0};
volatile unsigned char cache_ttl[CACHE_SPACE] = {0};
volatile unsigned int T0_Count = 0;
volatile unsigned int bisecondCount = 0;
volatile unsigned char modTemp = 0;


inline unsigned char setBit(unsigned char d,unsigned char n) {
	return (d | (1<<n));
}
/* Example : data = clearBit(data,1),means clear 1st bit of data(note data holds 1 byte) */
inline unsigned char clearBit(unsigned char d,unsigned char n) {
	return (d & (~(1<<n)));
}

/*
Structure for read button status
*/
volatile struct {
	unsigned bit4:1;
	unsigned bit5:1;
}bitVar;
/* Button Status Code */
/*
	0 Bluetoot
	1 GPRS
	2 RS485
*/
volatile unsigned char ButtonStatus = 1;//default for GPRS


/* Configurable Parameters */
/* Coordinator Parameters Default Value */
volatile unsigned char RouterNum = 5;//Totle Router Number in a zone
volatile unsigned char QueryPeriod = 30;//30 * 50 = 1500 ms = 1.5 second(Query Period)
/* Router Parameter Default Value */
volatile unsigned char CurrentThreshold = 20;
volatile unsigned char VoltageUpperRange = 20;//(240V)upper voltage range(+5%)
volatile unsigned char VoltageDownRange = 20;//(200V)down voltage range(-10%)
volatile unsigned char MonitorVoltageID = 1;//Router that sending Voltage Monitor Data
volatile unsigned char RetransmitTimeRatio = 50;//Retransmit period Ratio

/* Re-Query Missing Nodes */
volatile unsigned char missingNodeNumber;
volatile unsigned char nodeExistFlag[MaxRouterNum];

/*
** Initialize Button and Led Pins
*/
void initIO() {
	DDRC |= 0x80; //Makes bit 7 of PORTC Output(for LED)
	DDRC &= ~(0x30);//Makes bit 4 and bit 5 of PORTC Iutput(for Button)
	DDRD |= 0x30; //make PortD(4:5) output;(for CD4052)
}
/*
** Turn On LED
*/
void LEDON() {
	PORTC |= 0x80; //Turns ON LEDs
}
/*
** Turn Off LED
*/
void LEDOFF() {
	PORTC &= ~(0x80); //Turns OFF LEDs
}
/* 
** Read Switch Status
*/
void readButtonSatus() {
	volatile unsigned char temp;
	temp = PINC & 0x30;//button status

	ButtonStatus = ((temp & 0x10) << 1) + ((temp & 0x20) >> 1);
	ButtonStatus = ButtonStatus >> 4;

	bitVar.bit4 = temp >> 4;
	bitVar.bit5 = temp >> 5;
}
/* 
** Check Switch Status
*/
int checkStatus() {
	//if bit changed , then change the pin accordingly
	if(bitVar.bit5 ^ (PORTD & (1 << 5))) {
		if(bitVar.bit5) {
			PORTD = setBit(PORTD,5); //make output 10(connect to GPRS);
		}
		else {
			PORTD = clearBit(PORTD,5);
		}
	}

	//if bit changed , then change the pin accordingly
	if(bitVar.bit4 ^ (PORTD & (1 << 4))) {
		if(bitVar.bit4) {
			PORTD = setBit(PORTD,4);    	}
		else {
			PORTD = clearBit(PORTD,4);
		}
	}
	

	//if(bitVar.bit5 == 0) {
	//    /* Switch to 485 Bus */
	//    //PORTD = ;
	//} else {
	//    /* Switch to GPRS */
	//}

	if((~bitVar.bit4) && bitVar.bit5)return 1;
	else return 0;
}
/*
** USART0 Receive Interrupt Service Routing
*/
ISR(USART0_RX_vect)//USART Receive Complete Vector
{
	unsigned char temp;	
	UCSR0B &= (~(1<<RXCIE0));//disable receiver interrupt(reset bit)
	temp = UDR0;//read data
	/* Process Received Data that comes from Bluetooth */
	if((startFlag_Bluetooth == 1)&&(index_Bluetooth < recBufferSize_Bluetooth - 1)){
		recBuffer_Bluetooth[index_Bluetooth] = temp;
		index_Bluetooth++;
	}
	/* here we decide weather received data are valid */
	if(temp == StartByte_Bluetooth){
		startFlag_Bluetooth = 1;//when we received a start byte,set startFlag

		index_Bluetooth = 0;//initialize index_Zigbee,very important
	}
	else if((startFlag_Bluetooth == 1)&&(temp == EndByte_Bluetooth)){//endByte only make sense when startByte appeare
		startFlag_Bluetooth = 0;//when we received a end byte,reset startFlag
		recNum_Bluetooth = index_Bluetooth - 1;
		index_Bluetooth = 0;
		recFlag_Bluetooth = 1;
	}
	else{}

	UCSR0B |= (1<<RXCIE0);//re-enable receiver interrupt(set bit)
}
/*
USART1 Receive Interrupt Service Routing
*/
ISR(USART1_RX_vect)//USART Receive Complete Vector
{
	unsigned char temp;

	UCSR1B &= (~(1<<RXCIE1));//disable receiver interrupt(reset bit)
	temp = UDR1;//read data

	/* Overrun Error */
	/*if (DOR1 == 1) {
		temp = RCREG;
		temp = RCREG;
		CREN = 0;
		CREN = 1;
	}*/

	/* Process Received Data that comes from Zigbee */
	if((startFlag_Zigbee == 1)&&(index_Zigbee < recBufferSize_Zigbee - 1)){
		recBuffer_Zigbee[index_Zigbee] = temp;
		index_Zigbee++;
	}
	/* here we decide weather received data are valid */
	if(temp == StartByte_Zigbee){
		startFlag_Zigbee = 1;//when we received a start byte,set startFlag
		index_Zigbee = 0;//initialize index_Zigbee,very important
	}
	else if((startFlag_Zigbee == 1)&&(temp == EndByte_Zigbee)){//endByte only make sense when startByte appeare
		startFlag_Zigbee = 0;//when we received a end byte,reset startFlag
		recNum_Zigbee = index_Zigbee - 1;
		index_Zigbee = 0;
		recFlag_Zigbee = 1;
	}
	else{}
	//USART1_Send_Byte(temp);

	UCSR1B |= (1<<RXCIE1);//re-enable receiver interrupt(set bit)
}
/*
Timer0 Service Routing
*/
ISR(TIMER0_OVF_vect)//Timer0 Overflow Interrupt Vector
{
	/* Hardware will clear Interrupt Flag for us */
	T0_Count++;
	if(T0_Count >= 624) {//about 2 second
		T0_Count = 0;

		/* feed dog every 2 second */
		__asm__ __volatile__ ("wdr");//Watch Dog Timer Reset ??

		//process down operation per 2 seconds,so we can finish all down operation in 10 seconds
		modTemp = bisecondCount % CACHE_SPACE;//Router Number
		if(cache_ttl[modTemp] != 0) {
			/* Every Time we Just Substract 100 */
			if(cache_ttl[modTemp] >= 100)cache_ttl[modTemp] = cache_ttl[modTemp] - 100;
			else cache_ttl[modTemp] = 0;
		}

		++bisecondCount;

		if (bisecondCount >= CACHE_SPACE)bisecondCount = 0;//cuz the maximum retransmit time is about 300 seconds,we set clear cache circle abot 500 second

		//TIMSK0 = (0<<TOIE0);//DISABLE TIMER 0 OVERFLOW INTERRUPT(@page 111)
	}
	TCNT0 = T0IniVal;//Timing/Counter Register
}
/*
Store Received data into EEPROM
*/
void StoreZigbeeReceivedData()
{
	//unsigned char i;
	//unsigned char ReadTimeStatus;
	//unsigned char WriteEEPROMStatus;
	unsigned int temp;	
	/* default make data a current leakage data package */
	unsigned char DataType = 0x01;

	/* --- Step 2: After transmit ACK data ,read time ,then Archive the data along with ZigbBee data into EEPROM --- */  

	/* for cache data (big endian)*/
	if ((recBuffer_Zigbee[0] <= CACHE_SPACE) && (recBuffer_Zigbee[Zigbee_PackLen - 2] == 0x00))
	{
		/*	recBuffer_Zigbee[Zigbee_PackLen - 2 = 5] is a type indicator from ZigBee Router
		**	0x00 means abnormal data , and 0x01 means current leakage data , 0x03 means voltage monitor data
		**
		*/
		cache_ttl[recBuffer_Zigbee[0] % CACHE_SPACE] = CACHE_TIME;//unsigned char 0 ~ 256
		temp = recBuffer_Zigbee[1] * 256 + recBuffer_Zigbee[2];
		if(temp < CurrentThreshold) return; //invalid abnormal data

		cache_current[recBuffer_Zigbee[0]] = temp / 100;
		temp = recBuffer_Zigbee[3] * 256 + recBuffer_Zigbee[4];
		if(temp >= ((220 - VoltageDownRange) * 100) && temp <= ((220 + VoltageUpperRange) * 100)) return;// invalid abnormal data

		cache_voltage[recBuffer_Zigbee[0]] = temp / 100;
	}

	/* Check Weather we need to send data to GPRS or BlueTooth*/
	readButtonSatus();
	_delay_ms(1);//if status changes , we just need to delay for a while to fit it
	/* Now , we only consider condition of GPRS end device , so we don't check status */
	//if(checkStatus() > 0) {
	/* 0x03 means voltage monitor datas */
	if(recBuffer_Zigbee[Zigbee_PackLen - 2] == 0x03)DataType = 0xa0;//voltage monitor type data

	USART0_Send_Byte(StartByte_Zigbee);
	USART0_Send_Byte(recBuffer_Zigbee[0]);
	USART0_Send_Byte(recBuffer_Zigbee[1]);
	USART0_Send_Byte(recBuffer_Zigbee[2]);
	USART0_Send_Byte(recBuffer_Zigbee[3]);
	USART0_Send_Byte(recBuffer_Zigbee[4]);
	if(ButtonStatus == 0x00){
		USART0_Send_Byte(recBuffer_Zigbee[5]);
	} else {
		USART0_Send_Byte(DataType);//type indicator
	}
	USART0_Send_Byte(COORDINATORID);
	USART0_Send_Byte(EndByte_Zigbee);
}
/*
Read Command From Bluetooth
*/
void ReadCommandFromBluetooth() {
	volatile unsigned char i,j,k;
	volatile unsigned char count;
	volatile unsigned char ReadEEPROMStatus,date,month;
	volatile unsigned int ADDR_temp;
	//volatile unsigned int cache_temp;
	volatile unsigned char CommandByte = recData_Bluetooth[0];
	volatile unsigned char retryTime;

	/* Clear Data */
	recData_Bluetooth[0] = 0;

	switch(CommandByte){
	case 0x10://Read Unread data
		{	/** in this case , we just transmit unread data to BlueTooth end **/
			/**== the following program only changes 'FirstUnReadByteAddr' and 'LastReadByteAddr' ==**/
			/*=== first:transmit leakage data into BlueTooth ===*/
			//read unread data address
			date = ReadDS1307(DS1307,0x04);//read date,because we just transmit today's data
			month = ReadDS1307(DS1307,0x05);//read month

			LastUnReadByteAddr = 256*ReadEEPROM(AT24C128,21);//this address are stored in EEPROM(address:)High Byte
			LastUnReadByteAddr += ReadEEPROM(AT24C128,22);//Low Byte
			
			
			/** check back if there are some data to be read **/

			/* Convert Address to First-Some Address */
			if(LastUnReadByteAddr <= (ReservedByteNum - 1))
			ADDR_temp = EEpromSize - BlockLength;
			else 
			ADDR_temp = LastUnReadByteAddr - BlockLength + 1;//??
			
			count = 0;

			/* Read data and transmit it if they meet our need */
			while(1){
				
				count++;
				
				if(ADDR_temp <  ReservedByteNum)ADDR_temp = EEpromSize - BlockLength;
				/** when there are some data unread **/	
				ReadEEPROMStatus = Read_EEPROM_Block(AT24C128,ADDR_temp,R_EEprom_Array,BlockLength);
				/* Check weather the data is today received */
				if((R_EEprom_Array[7] != date)||(R_EEprom_Array[6] != month))break;
				
				if(count > 8)break;
				
				USART0_Send_Byte(StartByte_Zigbee);				
				for(j = 0;j < 6;j++)
				{
					USART0_Send_Byte(R_EEprom_Array[j]);			
				}
				_delay_ms(100);//added for debug@01-15
				for(j = 6;j < 13;j++)
				{
					USART0_Send_Byte(R_EEprom_Array[j]);			
				}
				_delay_ms(100);//added for debug@01-15
				for(j = 13;j < BlockLength;j++)
				{
					USART0_Send_Byte(R_EEprom_Array[j]);			
				}
				USART0_Send_Byte(EndByte_Zigbee);
				//_delay_ms(500);//added for debug@01-15
				ADDR_temp -= BlockLength;
			}
			USART0_Send_Byte(0xBB);//End byte

			/** acknowledgement(confirm data have been received by blueTooth end) **/
			/*while(Serial.available() <= 0)
			{delay(200);}//Waiting for data come from BlueTooth
				//delay(5000);//delay for 5 seconds
			// here we use if condition
			if(Serial.available() > 0){
							
			//Bluetooth_Temp = Serial.read();//read end byte of last package,and discard it.
						
			if(Bluetooth_Temp == StartByte_Bluetooth){
			ByteNum_Bluetooth = Serial.readBytesUntil(EndByte_Bluetooth, Bluetooth_ACK_Temp, 1);
			Bluetooth_Temp = 0;//very important
			}
			if((ByteNum_Bluetooth == 1)&&(Bluetooth_ACK_Temp[0] == (char)BlockNum)){//ack block length
			Bluetooth_Ack_Flag = true;
			ByteNum_Bluetooth = 0;
			Bluetooth_ACK_Temp[0] = 0x00;//very important
			//Serial.println("I am in here now!");//for test purpose
			//break;
			}
			//WriteDataFromEEpromToBluetooth(BlockNum,FirstUnReadByteAddr);
			//if no ACK received,treat it as nothing happened,we never retry it.
			//Serial.println(Bluetooth_ACK_Temp[0]);
			}
						
			//refresh read and unread data address,then write it into EEprom 
			//if(Bluetooth_Ack_Flag == 1){//if acknowledgement are success, then refresh address
				Bluetooth_Ack_Flag = 0;	
				FirstUnReadByteAddr = LastUnReadByteAddr + 1;
				LastReadByteAddr = LastUnReadByteAddr;
				Write2EEPROM(17,LastReadByteAddr>>8);//High Byte
				Write2EEPROM(18,LastReadByteAddr&0xFF);//Low Byte
				Write2EEPROM(19,FirstUnReadByteAddr>>8);//High Byte
				Write2EEPROM(20,FirstUnReadByteAddr&0xFF);//Low Byte
				//Serial.println("I am in here now!");
			}*/
			break;//(break out case condition)very important
		}
		/* Read All Restored Data */	
	case 0x20:
		{
			/* in this condition,we will transmit all leakage data into BlueTooth end, */
			/* which including unread and read data. */
			/* here no acknowledgement are needed because no need to change some thing in EEPROM*/
			/* besides,if user didn't receive data,he or she can retouch the icon. */
			FirstReadByteAddr = 256*ReadEEPROM(AT24C128,15);
			FirstReadByteAddr += ReadEEPROM(AT24C128,16);
			LastUnReadByteAddr = 256*ReadEEPROM(AT24C128,21);
			LastUnReadByteAddr += ReadEEPROM(AT24C128,22);
			EEpromFull =  ReadEEPROM(AT24C128,23);

			BlockNum = 0;//default no data are to be read
			if(LastUnReadByteAddr > FirstReadByteAddr)BlockNum = (LastUnReadByteAddr - FirstReadByteAddr + 1)/BlockLength;
			/* take '=' condition into consideration !! */
			if((LastUnReadByteAddr <= FirstReadByteAddr - 1) && (EEpromFull)){//when EEprom runs into circle storage state
				BlockNum = (EEpromSize-FirstReadByteAddr)/BlockLength +(LastUnReadByteAddr-(ReservedByteNum-1))/BlockLength;}
			
			/** check if there are some data to be read **/
			ADDR_temp = FirstReadByteAddr;
			for(i = 0;i < BlockNum;i++){
				if(ADDR_temp >= EEpromSize)ADDR_temp = ReservedByteNum;
				/** when there are some data unread **/	
				ReadEEPROMStatus = Read_EEPROM_Block(AT24C128,ADDR_temp,R_EEprom_Array,BlockLength);
				USART0_Send_Byte(StartByte_Zigbee);				
				for(j = 0;j < 6;j++)
				{
					USART0_Send_Byte(R_EEprom_Array[j]);			
				}
				_delay_ms(100);//added for debug@01-15
				for(j = 6;j < 13;j++)
				{
					USART0_Send_Byte(R_EEprom_Array[j]);			
				}
				_delay_ms(100);//added for debug@01-15
				for(j = 13;j < BlockLength;j++)
				{
					USART0_Send_Byte(R_EEprom_Array[j]);			
				}
				USART0_Send_Byte(EndByte_Zigbee);
				_delay_ms(100);//added for debug@01-15
				ADDR_temp += BlockLength;
			}
			USART0_Send_Byte(0xBB);//End byte
			BlockNum = 0;//reset block number
			break;//break out switch case (very important)
		}
		/* Read Data of all Routers Immediately */
	case 0x30:
		{
			/* Clear Missing Node Number */
			missingNodeNumber = 0;
			
			RealTimeQuery = 1;			
			/* Query Each Router */
			for(i = 1;i <= RouterNum;i++)//Start From 1
			{
				/* if the data is cached */
				if(cache_ttl[i] > 0) {
					/* Transmit cached data to Bluetooth end */
					_delay_ms(400);
					
					#ifdef DEBUG
					if (ButtonStatus == 0x00){
						USART0_Send_Byte(cache_ttl[i]);//_delay_ms(10);
					}
					#endif
					
					Send_Cached_Data_to_GPRS(i,\
											cache_current[i]*100,\
											cache_voltage[i]*100,\
											Normal_RealTime_Data,\
											ButtonStatus);
					/* Mark the node Exists */
					nodeExistFlag[i] = 1;
					//USART0_Send_Byte(0x11);	//for debug
					continue;//skip following code
				}
				
				for(retryTime = 0;retryTime < QUERYRETRYTIME;++retryTime) {
					Send_Query_Command_to_ZigBee(i);
					
					/* Wait for ACK */
					for(k = 0;k < QueryPeriod;k++) {
						if(1 == recFlag_Zigbee) break;
						else _delay_ms(50);
					}
					//_delay_ms(1000);//replace by for loop up there
					if(1 == recFlag_Zigbee) {
						recFlag_Zigbee = 0;
						/* --- Step 1: Send ACK_Zigbee to ZigBee router --- */
						/* then router stop send data to coordinator */
						ACK_Zigbee[1] = recBuffer_Zigbee[0];//router device id
						ACK_Zigbee[2] = recBuffer_Zigbee[1];//leak current high byte
						ACK_Zigbee[3] = recBuffer_Zigbee[2];//leak current low byte
						for(j = 0;j < Zigbee_AckLen;j++) {	//you can no longer use variable i,so we choose j
							USART1_Send_Byte(ACK_Zigbee[j]);
						}
						//subtract start and end byte of received data
						
						//if((recNum_Zigbee == (Zigbee_PackLen - 2))&&(recBuffer_Zigbee[0] == i)&&(RealTimeQuery == 1))
						//{
						/* Transmit Received data to Bluetooth end */
						//_delay_ms(1000);
						Send_Received_Data_to_GPRS(	recBuffer_Zigbee,\
													sizeof(recBuffer_Zigbee) / sizeof(unsigned char),\
													recBuffer_Zigbee[0],\
													Normal_RealTime_Data,\
													ButtonStatus);
						/* Mark node Exists */
						nodeExistFlag[i] = 1;
						_delay_ms(400);
						/* Break Retransmit For Loop */
						break;
						//}
					}
					/* Send Default Value at the Last Query Retry */
					else if(retryTime == QUERYRETRYTIME - 1)//If no Ack are Received(retry just 1 time!)@version3
					{
						/* Transmit Received data to Bluetooth end */
						#ifdef DEBUG
						if (ButtonStatus == 0x00) {
							USART0_Send_Byte(0x88);
						}
						#endif
						Send_Received_Data_to_GPRS(	null,\
													sizeof(recBuffer_Zigbee) / sizeof(unsigned char),\
													i,\
													Normal_RealTime_Data,\
													ButtonStatus);
						/* Mark node not Exists */
						nodeExistFlag[i] = 0;
					}
				}//end of retry for loop
			}//end of for loop
			
			
			#ifdef RETRY_MISSING_NODE
			/* Query missing nodes */
			for(i = 1;i <= RouterNum;i++)//Start From 1
			{
				/* Skip node exists */
				if(nodeExistFlag[i] == 1) {
					/* Reset Flags */
					nodeExistFlag[i] = 0;
					continue;
				}
				/* if the data is cached */
				if(cache_ttl[i] > 0) {
					/* Transmit cached data to Bluetooth end */
					_delay_ms(400);
					
					#ifdef DEBUG
					if (ButtonStatus == 0x00){
						USART0_Send_Byte(cache_ttl[i]);//_delay_ms(10);
					}
					#endif
					
					Send_Cached_Data_to_GPRS(i,\
											cache_current[i]*100,\
											cache_voltage[i]*100,\
											Normal_RealTime_Data,\
											ButtonStatus);

					//USART0_Send_Byte(0x11);	//for debug
					continue;//skip following code
				}
				
				for(retryTime = 0;retryTime < QUERYRETRYTIME;++retryTime) {
					Send_Query_Command_to_ZigBee(i);
					
					/* Wait for ACK */
					for(k = 0;k < QueryPeriod;k++) {
						if(1 == recFlag_Zigbee) break;
						else _delay_ms(50);
					}
					//_delay_ms(1000);//replace by for loop up there
					if(1 == recFlag_Zigbee) {
						recFlag_Zigbee = 0;
						/* --- Step 1: Send ACK_Zigbee to ZigBee router --- */
						/* then router stop send data to coordinator */
						ACK_Zigbee[1] = recBuffer_Zigbee[0];//router device id
						ACK_Zigbee[2] = recBuffer_Zigbee[1];//leak current high byte
						ACK_Zigbee[3] = recBuffer_Zigbee[2];//leak current low byte
						for(j = 0;j < Zigbee_AckLen;j++) {	//you can no longer use variable i,so we choose j
							USART1_Send_Byte(ACK_Zigbee[j]);
						}
						//subtract start and end byte of received data
						
						//if((recNum_Zigbee == (Zigbee_PackLen - 2))&&(recBuffer_Zigbee[0] == i)&&(RealTimeQuery == 1))
						//{
						/* Transmit Received data to Bluetooth end */
						//_delay_ms(1000);
						Send_Received_Data_to_GPRS(	recBuffer_Zigbee,\
													sizeof(recBuffer_Zigbee) / sizeof(unsigned char),\
													recBuffer_Zigbee[0],\
													Normal_RealTime_Data,\
													ButtonStatus);
						_delay_ms(400);
						/* Break Retransmit For Loop */
						break;
						//}
					}
					/* Send Default Value at the Last Query Retry */
					else if(retryTime == QUERYRETRYTIME - 1)//If no Ack are Received(retry just 1 time!)@version3
					{
						/* Transmit Received data to Bluetooth end */
						#ifdef DEBUG
						if (ButtonStatus == 0x00) {
							USART0_Send_Byte(0x88);
						}
						#endif
						Send_Received_Data_to_GPRS(	null,\
													sizeof(recBuffer_Zigbee) / sizeof(unsigned char),\
													i,\
													Normal_RealTime_Data,\
													ButtonStatus);
					}
				}//end of retry for loop
			}//end of for loop
				
			#endif
			
			
			/* The Last Packet */
			_delay_ms(500);
			if (0x01 == ButtonStatus) {
				Send_Received_Data_to_GPRS(	null,\
											sizeof(recBuffer_Zigbee) / sizeof(unsigned char),\
											i,\
											Last_RealTime_Data,\
											ButtonStatus);
			}
			
			RealTimeQuery = 0;
			break;
		}// case 0x30
		
	default:{
			//no ideal what to do yet!
		}
		
	}//end of switch function
	
	/* Query Data of Specified Router(Node) Immediately */
	if(CommandByte > 0x30 && CommandByte - 0x30 <= RouterNum) {
		RealTimeQuery = 1;
		/* if the data is cached */
		if(cache_ttl[CommandByte - 0x30] > 0) {
			/* Transmit cached data to Bluetooth end */
			#ifdef DEBUG
			if (ButtonStatus == 0x00){
				USART0_Send_Byte(cache_ttl[i]);//_delay_ms(10);
			}
			#endif
			
			Send_Cached_Data_to_GPRS(CommandByte - 0x30,\
									cache_current[CommandByte - 0x30] * 100,\
									cache_voltage[CommandByte - 0x30] * 100,\
									Normal_RealTime_Data,\
									ButtonStatus);
									
		}
		else {
			/* Query Certain Router */
			/* Send Query Command to Routers */	
			
			Send_Query_Command_to_ZigBee(CommandByte - 0x30);

			/* Wait for ACK */
			for(k = 0;k < QueryPeriod;k++) {
				if(1 == recFlag_Zigbee) break;
				else _delay_ms(50);
			}
			/* Check Receive Buffer */
			if(1 == recFlag_Zigbee) {
				recFlag_Zigbee = 0;
				/* --- Step 1: Send ACK_Zigbee to ZigBee router --- */
				/* then router stop send data to coordinator */
				ACK_Zigbee[1] = recBuffer_Zigbee[0];//router device id
				ACK_Zigbee[2] = recBuffer_Zigbee[1];//leak current high byte
				ACK_Zigbee[3] = recBuffer_Zigbee[2];//leak current low byte
				for(i = 0;i < Zigbee_AckLen;i++)
				{
					USART1_Send_Byte(ACK_Zigbee[i]);
				}
				//subtract start and end byte of received data
				//if((recNum_Zigbee == (Zigbee_PackLen - 2)) && (recBuffer_Zigbee[0] == (CommandByte - 0x30)) && (RealTimeQuery == 1))
				//{
				/* Transmit Received data to Bluetooth end */
				Send_Received_Data_to_GPRS(	recBuffer_Zigbee,\
											sizeof(recBuffer_Zigbee) / sizeof(unsigned char),\
											recBuffer_Zigbee[0],\
											Normal_RealTime_Data,\
											ButtonStatus);
				
				//}
			}
			else { //If no Ack are Received
				/* Transmit Received data to Bluetooth end */
				#ifdef DEBUG
				if (ButtonStatus == 0x00){
					USART0_Send_Byte(0x88);//_delay_ms(10);
				}
				#endif
				
				Send_Received_Data_to_GPRS(	null,\
											sizeof(recBuffer_Zigbee) / sizeof(unsigned char),\
											CommandByte - 0x30,\
											Normal_RealTime_Data,\
											ButtonStatus);
			}
		}
		_delay_ms(500);
		/* The Last Packet */
		if(0x01 == ButtonStatus) {
			
			Send_Received_Data_to_GPRS(	null,\
										sizeof(recBuffer_Zigbee) / sizeof(unsigned char),\
										CommandByte - 0x30,\
										Last_RealTime_Data,\
										ButtonStatus);
		}

		RealTimeQuery = 0;
	}// end of single node query if statement
	
	//CommandByte = 0;//CLear Buffer
}

void CheckParameter(){
	/* address 10 is the flag byte , indicate that if we already write on-chip eeprom */
	if(eeprom_read_byte((uint8_t *)10) == 0x55) {
		/* we already configed parameters , so use these parameters */
		RouterNum = eeprom_read_byte((uint8_t *)0);
		QueryPeriod = eeprom_read_byte((uint8_t *)1);
		CurrentThreshold = eeprom_read_byte((uint8_t *)2);
		VoltageUpperRange = eeprom_read_byte((uint8_t *)3);
		VoltageDownRange = eeprom_read_byte((uint8_t *)4);
		MonitorVoltageID = eeprom_read_byte((uint8_t *)5);
		RetransmitTimeRatio = eeprom_read_byte((uint8_t *)6);
	}
}

int main()
{
	volatile unsigned char i = 0, j = 0;
	//volatile unsigned char r_staus;
	volatile unsigned char t;
	
	cli();

	/* Initialization */
	TWI_Init();
	USART0_Init(38400);//Initialize USART0 with baud rate of 38400
	USART1_Init(38400);//Initialize USART1 with baud rate of 38400
	Timer0_Init();
	InitWatchDogTimer();
	initIO();
	CheckParameter();
	
	sei();            //Enable Gloabal Interrupt
	_delay_ms(50);

	#ifdef DEBUG
	USART0_Send_Byte(0x55);//for debug Watch Dog Timer
	#endif

	while(1) {
		//read switch button status
		readButtonSatus();
		t = checkStatus();
		/* If Valid Data have been Received From Zigbee */
		if(1 == recFlag_Zigbee) {
			cli();	//clear global interrupt
			recFlag_Zigbee = 0;
			/* --- Step 1: Send ACK_Zigbee to ZigBee router --- */
			/* then router stop send data to coordinator */
			ACK_Zigbee[1] = recBuffer_Zigbee[0];//router device id
			ACK_Zigbee[2] = recBuffer_Zigbee[1];//leak current high byte
			ACK_Zigbee[3] = recBuffer_Zigbee[2];//leak current low byte
			for(i = 0;i < Zigbee_AckLen;i++) {
				/* Send Acknowledgement Packet to Router */
				USART1_Send_Byte(ACK_Zigbee[i]);
			}
			/* Store Received Data to EEPROM */
			if(recNum_Zigbee == (Zigbee_PackLen - 1)) {//added 1 byte (07-15-2015)
				
				LEDON();
				if(RealTimeQuery == 0) StoreZigbeeReceivedData();//Ignore Query Staus
				LEDOFF();
				//USART0_Send_Byte(0x35);
				//USART0_Send_Byte(0x0A);
			}

			/* clear receive buffer @ 06_09 */
			for (i = 0; i < recBufferSize_Zigbee; ++i) {
				recBuffer_Zigbee[i] = 0;
			}

			sei();	//set global interrupt
		}
		/* If Valid Data have been Received From Bluetooth */
		if(1 == recFlag_Bluetooth) {
			recFlag_Bluetooth = 0;
			LEDON();
			/* Copy data from buffer to a array */
			for(j = 0;j < recNum_Bluetooth;++j) {
				recData_Bluetooth[j] = recBuffer_Bluetooth[j];
				/* Clear Data */
				recBuffer_Bluetooth[j] = 0;
			}

			//subtract start and end byte of received data
			if (1 == recNum_Bluetooth) {
				/* if Received Data Transfer Command*/
				ReadCommandFromBluetooth();	
			} else if (7 == recNum_Bluetooth) {
				/* if Received Parameter Configuring Command */

				/* Parsing Parameter */
				RouterNum = recData_Bluetooth[0];
				QueryPeriod = recData_Bluetooth[1];//default for 30
				/* Router Parameter */
				CurrentThreshold = recData_Bluetooth[2];
				VoltageUpperRange = recData_Bluetooth[3];//(240V)upper voltage range(+5%)
				VoltageDownRange = recData_Bluetooth[4];//(200V)down voltage range(-10%)
				MonitorVoltageID = recData_Bluetooth[5];
				RetransmitTimeRatio = recData_Bluetooth[6];//default for 50
				/* Maybe We Could Add a Check-Sum Byte */
				
				/* Write data to on-chip eeprom */
				eeprom_write_byte((uint8_t *)0,RouterNum);
				eeprom_write_byte((uint8_t *)1,QueryPeriod);
				eeprom_write_byte((uint8_t *)2,CurrentThreshold);
				eeprom_write_byte((uint8_t *)3,VoltageUpperRange);
				eeprom_write_byte((uint8_t *)4,VoltageDownRange);
				eeprom_write_byte((uint8_t *)5,MonitorVoltageID);
				eeprom_write_byte((uint8_t *)6,RetransmitTimeRatio);
				eeprom_write_byte((uint8_t *)10,0x55);


				/* Sending Parameters */
				USART1_Send_Byte(StartByte_Zigbee);//_delay_ms(10);
				USART1_Send_Byte(0xD0);
				USART1_Send_Byte(CurrentThreshold);
				USART1_Send_Byte(VoltageUpperRange);
				USART1_Send_Byte(VoltageDownRange);
				USART1_Send_Byte(MonitorVoltageID);
				USART1_Send_Byte(RetransmitTimeRatio);
				USART1_Send_Byte(EndByte_Zigbee);
			} else if (2 == recNum_Bluetooth){
				//for debug(sending ButtonStatus)
				if(recData_Bluetooth[0] == 0x01) {
					USART0_Send_Byte(ButtonStatus);
				} else if(recData_Bluetooth[0] == 0x02) {
					USART0_Send_Byte(RouterNum);//_delay_ms(10);
					USART0_Send_Byte(QueryPeriod);
					USART0_Send_Byte(CurrentThreshold);
					USART0_Send_Byte(VoltageUpperRange);
					USART0_Send_Byte(VoltageDownRange);
					USART0_Send_Byte(MonitorVoltageID);
					USART0_Send_Byte(RetransmitTimeRatio);
					USART0_Send_Byte(eeprom_read_byte((uint8_t *)10));
				} else if(recData_Bluetooth[0] == 0x03){
					USART0_Send_Byte(cache_ttl[recData_Bluetooth[1]]);
				}
			}

			LEDOFF();
		}

		//_delay_ms(1);//why delay?
	}
	return 0;
}

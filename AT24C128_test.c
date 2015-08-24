/*
Here, batch read/write function are added
*/

#define F_CPU 16000000UL  /* 16 MHz CPU clock */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define DS1307 0x68//define slave device address.
#define AT24C128 0x50

/*----------  TWI related Macro ---------------*/
/* @ datasheet page 222 */
/*----------  Variable Macro ---------------*/
#define  START  0X08//A TWI_Start condition has been transmitted
#define  ReStart 0x10//A repeated Start condition has been transmitted
#define  MT_SLA_ACK  0X18//(Master Transmit SLave Address ACK)slave+w has been transmitted;ACK has been received
#define  MR_SLA_ACK 0x40//Master Receive Slave Address ACK
#define  MT_DATA_ACK  0X28//(Master Transmit DATA ACK)data byte has been transmitted;ACK has been received
#define  MR_DATA_ACK 0x50//Master Receive DATA ACK
#define  MR_DATA_NACK 0x58//Master Receive DATA not ACK

#define Read 1
#define Write 0
/*---------- Function Macro ---------------*/
#define Start() (TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN))	//产生START信号
#define Stop() (TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN))	//产生STOP信号
#define Wait() while(!(TWCR&(1<<TWINT)))				//等待当前操作完成
#define TestACK() (TWSR&0xF8)							//取出状态码(TWI Status Register)
#define SetACK() (TWCR|=(1<<TWEA))						//产生ACK
#define ResetACK() (TWCR=(1<<TWINT)|(1<<TWEN))			//产生NACK
#define Writebyte(twi_d) {TWDR=(twi_d);TWCR=(1<<TWINT)|(1<<TWEN);}	//发送一个字节（twi_d为写入的数据）

#define BlockNum 15

volatile unsigned char rdata;
volatile unsigned char flag = 0;
/*
Initialize TWI
*/
void TWI_Init()
{
	/* @page 234 */
    TWBR=0X20;	//TWI Bit Rate Register
	TWCR=0X44;	//TWI Control Register
	TWSR=0x00;	//TWI Status Register
}
/*
Write 1 byte data to EEPROM
*/
unsigned char WriteEEPROM(unsigned char DevAddr,unsigned int MemAddr,unsigned char data)
{
	/* Start TWI */
	Start();
	Wait();
	if(TestACK()!=START)
	{
	   return 0;
	}
	/* Write Device Address */
	Writebyte((DevAddr<<1)|(Write));
	Wait();
	if(TestACK()!=MT_SLA_ACK)
	{
	   return 0;
	}
	/* Write Memory Address High Byte) */
	Writebyte(MemAddr>>8);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
		return 0;
	}
	/* Write Memory Address Low Byte) */
	Writebyte(MemAddr&0xff);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
		return 0;
	}
	/* Write Data to EEPROM */
	Writebyte(data);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
	   return 0;
	}
	Stop();
	_delay_ms(10);
	
	return 1;
}
/*
Write n byte data to EEPROM
*/
unsigned char Write_EEPROM_Block(unsigned char DevAddr,unsigned int MemAddr,unsigned char *p,unsigned char num)
{
	unsigned char i;	
	/* Start TWI */
	Start();
	Wait();
	if(TestACK()!=START)
	{
	   return 0;
	}
	/* Write Device Address */
	Writebyte((DevAddr<<1)|(Write));
	Wait();
	if(TestACK()!=MT_SLA_ACK)
	{
	   return 0;
	}
	/* Write Memory Address High Byte) */
	Writebyte(MemAddr>>8);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
		return 0;
	}
	/* Write Memory Address Low Byte) */
	Writebyte(MemAddr&0xff);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
		return 0;
	}
	/* Write Data to EEPROM */
	for(i = 0;i < num;i++){
		Writebyte(*(p+i));
		Wait();
		if(TestACK()!=MT_DATA_ACK)
		{
	   		return 0;
		}
	}

	Stop();
	_delay_ms(10);
	
	return 1;
}
/*
Read 1 byte data from EEPROM
*/
unsigned char ReadEEPROM(unsigned char DevAddr,unsigned int MemAddr)
{
	unsigned char data;
	/* Start TWI */
	Start();
	Wait();
	if(TestACK()!=START)
	{
		return 0;
	}	
	/* Write Device Address */
	Writebyte((DevAddr<<1)|(Write));//SLA+W
	Wait();
	if(TestACK()!=MT_SLA_ACK)
	{
		return 0;
	}
	/* Write Memory Address High Byte) */
	Writebyte(MemAddr>>8);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
		return 0;
	}
	/* Write Memory Address Low Byte) */
	Writebyte(MemAddr&0xff);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
		return 0;
	}
	/* Restart */	
	Start();
	Wait();
	if(TestACK()!=ReStart)
	{
		return 0;
	}
	/* Write Device Address(read format) */
	Writebyte((DevAddr<<1)|(Read));//SLW+R
	Wait();
	if(TestACK()!=MR_SLA_ACK)//
	{
		return 0;
	}

	/* added for certian purpose */
	TWCR=(1<<TWINT)|(1<<TWEN);
	Wait();

	/* Receive Data(1 byte only) */

	data = TWDR;

	/* Stop TWI */

	Stop();

	_delay_ms(5);

	return data;
}
/*
Write n byte data to EEPROM
*/
unsigned char Read_EEPROM_Block(unsigned char DevAddr,unsigned int MemAddr,unsigned char *p,unsigned char num)
{
	//unsigned char data;
	unsigned char i = 0;
	
	DevAddr = (DevAddr<<1)|(Write);//????????????????
	/* Start TWI */
	Start();
	Wait();
	if(TestACK()!=START)
	{
	   return 0;
	}
	/* Write Device Address */
	Writebyte(DevAddr);//SLA+W
	Wait();
	if(TestACK()!=MT_SLA_ACK)
	{
	   return 0;
	}
	/* Write Memory Address High Byte) */
	Writebyte(MemAddr>>8);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
		return 0;
	}
	/* Write Memory Address Low Byte) */
	Writebyte(MemAddr&0xff);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
		return 0;
	}
	/* Restart */	
	Start();
	Wait();
	if(TestACK()!=ReStart)
	{
	   return 0;
	}
	/* Write Device Address(read format) */
	Writebyte(DevAddr + 1);//SLW+R
	Wait();
	if(TestACK()!=MR_SLA_ACK)//
	{
	   return 0;
	}
	/* Read MultiByte From EEPROM */
	for(i = 0;i < (num-1);i++){
		/* Receive Data(1 byte only) */
		SetACK();
		Wait();
		if(TestACK()!=MR_DATA_ACK)//
		{
	   		return 0;
		}
		*(p+i) = TWDR;
		//to do (ACK)????	
	}
	ResetACK();
	Wait();
	if(TestACK()!=MR_DATA_NACK)//
	{
	   	return 0;
	}
	*(p+num-1) = TWDR;

	Stop();

	_delay_ms(5);
	
	return 1;

}
/*
Initialize USART0
*/
void USART0_Init(unsigned int baud)
{
	UCSR0A = 0x00;//defalut value
	UCSR0B = 0x00;//USART Control and Status Register B 		    //控制寄存器清零
	UCSR0C = 3<<UCSZ00;//8 bit data
                                                        //选择UCSRC，异步模式，禁止                        
                                                     //   校验，1位停止位，8位数据位
	baud = F_CPU/16/baud - 1	;   //波特率最大为65K
	UBRR0L = baud; 					     	  
	UBRR0H = baud>>8; 		   //设置波特率
   
	UCSR0B = (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0); //接收、发送使能，接收中断使能
   
	//SREG = BIT(7);	       //全局中断开放
	DDRD |= 0x02;	           //配置TX0 pin(PD1) 为输出（很重要）
}
/*
Initialize USART1
*/
void USART1_Init(unsigned int baud)
{
	UCSR1A = 0x00;
	UCSR1B = 0x00;//USART Control and Status Register B 		    //控制寄存器清零
	UCSR1C = 3<<UCSZ10;//8 bit data
                                                        //选择UCSRC，异步模式，禁止                        
                                                     //   校验，1位停止位，8位数据位
	baud = F_CPU/16/baud - 1	;   //波特率最大为65K
	UBRR1L = baud; 					     	  
	UBRR1H = baud>>8; 		   //设置波特率
	
	UCSR1B = (1<<TXEN1)|(1<<RXEN1)|(1<<RXCIE1); //接收、发送使能，接收中断使能
   	
	//sei();                     //Enable Gloabal Interrupt
	//SREG = BIT(7);	       //全局中断开放
	DDRD |= 0x08;	           //配置TX1 pin(PD3) 为输出（很重要）
}
/*
Send 1 BYTE Data Through USART0
*/
void USART0_Send_Byte(unsigned char data)
{
	/* waitting for a empty USART Data Register */
	while(!(UCSR0A&(1<<UDRE0))) ;
	UDR0 = data;//USART Data Register
   
	/* waitting for USART Transmit Complete */
	while(!(UCSR0A&(1<<TXC0)));
	UCSR0A |= 1<<TXC0;//set TXC bit manually
}
/*
Send 1 BYTE Data Through USART1
*/
void USART1_Send_Byte(unsigned char data)
{
	/* waitting for a empty USART Data Register */
	while(!(UCSR1A&(1<<UDRE1))) ;
	UDR1 = data;//USART Data Register
   
	/* waitting for USART Transmit Complete */
	while(!(UCSR1A&(1<<TXC1)));
	UCSR1A |= 1<<TXC1;//set TXC bit manually
}
/*
USART0 Receive Interrupt Service Routing
*/
ISR(USART0_RX_vect)//USART Receive Complete Vector
{
	unsigned char temp;	
	UCSR0B &= (~(1<<RXCIE0));//disable receiver interrupt(reset bit)
	temp = UDR0;//read data
	//flag = 1;//set receive flag
	
	/* waitting for a empty USART Data Register */
	while(!(UCSR0A&(1<<UDRE0))) ;
	UDR0 = temp;//USART Data Register
   
	/* waitting for USART Transmit Complete */
	while(!(UCSR0A&(1<<TXC0)));
	UCSR0A |= 1<<TXC0;//set TXC bit manually

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
	//flag = 1;//set receive flag
	
	/* waitting for a empty USART Data Register */
	while(!(UCSR1A&(1<<UDRE1))) ;
	UDR1 = temp;//USART Data Register
   
	/* waitting for USART Transmit Complete */
	while(!(UCSR1A&(1<<TXC1)));
	UCSR1A |= 1<<TXC1;//set TXC bit manually

	UCSR1B |= (1<<RXCIE1);//re-enable receiver interrupt(set bit)
}
void Init()
{
	WriteEEPROM(AT24C128,0,1);//Coordinator ID - 1
	WriteEEPROM(AT24C128,15,0);//high byte of FirstReadByteAddr
	WriteEEPROM(AT24C128,16,34);//low byte of FirstReadByteAddr
	WriteEEPROM(AT24C128,17,0);//high byte of LastReadByteAddr
	WriteEEPROM(AT24C128,18,33);//low byte of LastReadByteAddr
	WriteEEPROM(AT24C128,19,0);//high byte of  FirstUnReadByteAddr
	WriteEEPROM(AT24C128,20,34);//low byte of FirstUnReadByteAddr
	WriteEEPROM(AT24C128,21,0);//high byte of LastUnReadByteAddr
	WriteEEPROM(AT24C128,22,33);//low byte of LastUnReadByteAddr
	WriteEEPROM(AT24C128,23,0);//EEPROM Full Byte

	/* 16323 */
	//WriteEEPROM(AT24C128,17,0x3F);//high byte of LastReadByteAddr
	//WriteEEPROM(AT24C128,18,0xC3);//low byte of LastReadByteAddr

	/* 16324 */
	//WriteEEPROM(AT24C128,19,0x3F);//high byte of  FirstUnReadByteAddr
	//WriteEEPROM(AT24C128,20,0xC4);//low byte of FirstUnReadByteAddr

	/* 16323 */
	//WriteEEPROM(AT24C128,21,0x3F);//high byte of LastUnReadByteAddr
	//WriteEEPROM(AT24C128,22,0xC3);//low byte of LastUnReadByteAddr

	//WriteEEPROM(AT24C128,23,0);//EEPROM Full Byte
}
void ReadAddr()
{
	unsigned int temp;

	USART0_Send_Byte(0x30 + ReadEEPROM(AT24C128,0));
	USART0_Send_Byte(0x0A);
	
	temp = 256*ReadEEPROM(AT24C128,15) + ReadEEPROM(AT24C128,16);

	USART0_Send_Byte(0x30 + temp/10000);
	USART0_Send_Byte(0x30 + (temp%10000)/1000);
	USART0_Send_Byte(0x30 + (temp%1000)/100);
	USART0_Send_Byte(0x30 + (temp%100)/10);
	USART0_Send_Byte(0x30 + (temp%10));
	USART0_Send_Byte(0x0A);//line feed
	_delay_ms(5);

	temp = 256*ReadEEPROM(AT24C128,17) + ReadEEPROM(AT24C128,18);

	USART0_Send_Byte(0x30 + temp/10000);
	USART0_Send_Byte(0x30 + (temp%10000)/1000);
	USART0_Send_Byte(0x30 + (temp%1000)/100);
	USART0_Send_Byte(0x30 + (temp%100)/10);
	USART0_Send_Byte(0x30 + (temp%10));
	USART0_Send_Byte(0x0A);//line feed
	_delay_ms(5);

	temp = 256*ReadEEPROM(AT24C128,19) + ReadEEPROM(AT24C128,20);

	USART0_Send_Byte(0x30 + temp/10000);
	USART0_Send_Byte(0x30 + (temp%10000)/1000);
	USART0_Send_Byte(0x30 + (temp%1000)/100);
	USART0_Send_Byte(0x30 + (temp%100)/10);
	USART0_Send_Byte(0x30 + (temp%10));
	USART0_Send_Byte(0x0A);//line feed
	_delay_ms(5);

	temp = 256*ReadEEPROM(AT24C128,21) + ReadEEPROM(AT24C128,22);

	USART0_Send_Byte(0x30 + temp/10000);
	USART0_Send_Byte(0x30 + (temp%10000)/1000);
	USART0_Send_Byte(0x30 + (temp%1000)/100);
	USART0_Send_Byte(0x30 + (temp%100)/10);
	USART0_Send_Byte(0x30 + (temp%10));
	USART0_Send_Byte(0x0A);//line feed
	_delay_ms(5);

	USART0_Send_Byte(0x30 + ReadEEPROM(AT24C128,23));
	USART0_Send_Byte(0x0A);//line feed
}

int main()
{
    //unsigned char temp = 0;
    unsigned char second;
    unsigned char j = 0;
    unsigned char temp_data,memData;
    unsigned char w_array[BlockNum];
    unsigned char r_array[BlockNum];	
    unsigned int addr;

    USART0_Init(38400);//Initialize USART0 with baud rate of 9600
    USART1_Init(38400);//Initialize USART1 with baud rate of 9600
	
    TWI_Init();
    sei();         	//Enable Gloabal Interrupt
    //cli();		//Disable Gloabal Interrupt
    _delay_ms(500);
        
    /* Address Range : 0 ~ 16384 - 1 */
    addr = 2 + 16384;
    temp_data = 0x55;
    //WriteEEPROM(AT24C128,addr,temp_data);
    temp_data = ReadEEPROM(AT24C128,addr);
    USART0_Send_Byte(temp_data);

    return 0;
}

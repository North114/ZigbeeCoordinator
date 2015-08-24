/*
Here, batch read/write function are added
*/
#define F_CPU 16000000UL  /* 16 MHz CPU clock */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define DS1307 0x68//define slave device address.
#define AT24C128 0x50

/* @ datasheet page 222 */
/* status codes for master transmiter mode */
#define  START  0X08//A TWI_Start condition has been transmitted
#define  ReStart 0x10//A repeated Start condition has been transmitted
#define  MT_SLA_ACK  0X18//(Master Transmit SLave Address ACK)slave+w has been transmitted;ACK has been received
#define  MR_SLA_ACK 0x40//Master Receive Slave Address ACK
#define  MT_DATA_ACK  0X28//(Master Transmit DATA ACK)data byte has been transmitted;ACK has been received
#define  MR_DATA_ACK 0x50//Master Receive DATA ACK
#define  MR_DATA_NACK 0X58//Master Receive DATA Not ACK

#define Read 1
#define Write 0

#define Start() (TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN))	//产生START信号
#define Stop() (TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN))	//产生STOP信号
#define Wait() while(!(TWCR&(1<<TWINT)))				//等待当前操作完成
#define TestACK() (TWSR&0xF8)							//取出状态码(TWI Status Register)
#define SetACK() (TWCR|=(1<<TWEA))						//产生ACK
#define ResetACK() (TWCR=(1<<TWINT)|(1<<TWEN))			//产生NACK
#define Writebyte(twi_d) {TWDR=(twi_d);TWCR=(1<<TWINT)|(1<<TWEN);}	//发送一个字节（twi_d为写入的数据）

volatile unsigned char rdata;
volatile unsigned char flag = 0;

unsigned char CurrentTime[7];

void TWI_Init()
{
	/* @page 234 */
    TWBR=0x20;	//TWI Bit Rate Register
	TWCR=0x44;	//TWI Control Register
	TWSR=0;		//TWI Status Register
}

unsigned char WriteEEPROM(unsigned char devadd,unsigned char cmd,unsigned char rdata)
{
	devadd = (devadd<<1)|(Write); 
	Start();
	Wait();
	if(TestACK()!=START)
	{
	   return 0;
	}
	Writebyte(devadd);
	Wait();
	if(TestACK()!=MT_SLA_ACK)
	{
	   return 0;
	}
	Writebyte(cmd);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
	   return 0;
	}
	Writebyte(rdata);
	Wait();
	if(TestACK()!=MT_DATA_ACK)
	{
	   return 0;
	}
	Stop();
	_delay_ms(10);
	
	return 1;
}
unsigned char ReadEEPROM(int addr){

return 1;
}

/*
Read All Time data
*/
unsigned char Read_Current_Time(unsigned char DevAddr,unsigned char *p,unsigned char num)
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
	/* Write Register Address 0x00) */
	Writebyte(0);
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
	Writebyte(DevAddr + 1);//SLW+R
	Wait();
	if(TestACK()!=MR_SLA_ACK)//
	{
	   return 0;
	}

	/* Rstart TWI */
	//TWCR=(1<<TWINT)|(1<<TWEN);
	//Wait();

	/* Read MultiByte From DS1307 ???? */
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
Read 1 data byte from DS1307
*/
unsigned char ReadDS1307(unsigned char DevAddr,unsigned char RegAddr)
{
	unsigned char data;
	
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
	/* Write Register Address 0x00) */
	Writebyte(RegAddr);
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
	Writebyte(DevAddr + 1);//SLW+R
	Wait();
	if(TestACK()!=MR_SLA_ACK)//
	{
	   return 0;
	}
	
	/* added for certian purpose */
	//TWCR=(1<<TWINT)|(1<<TWEN);
	ResetACK();
	Wait();
	if(TestACK()!=MR_DATA_NACK)//
	{
	   return 0;
	}
	/* Receive Data(1 byte only) */
	data = TWDR;
	
	Stop();

	_delay_ms(5);
	
	return data;

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
   
	sei();                     //Enable Gloabal Interrupt
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

void InitTime()
{
	WriteEEPROM(DS1307,0x00,0x00);
	_delay_ms(10);
	WriteEEPROM(DS1307,0x01,0x31);
	_delay_ms(10);
	WriteEEPROM(DS1307,0x02,0x22);
	_delay_ms(10);
	WriteEEPROM(DS1307,0x03,0x01);
	_delay_ms(10);
	WriteEEPROM(DS1307,0x04,0x13);
	_delay_ms(10);
	WriteEEPROM(DS1307,0x05,0x07);
	_delay_ms(10);
	WriteEEPROM(DS1307,0x06,0x15);
	_delay_ms(10);
}

int main()
{
    //unsigned char temp = 0;
	unsigned char second;
	unsigned char r;
	USART0_Init(38400);//Initialize USART0 with baud rate of 9600
	USART1_Init(38400);//Initialize USART1 with baud rate of 9600
	
	TWI_Init();

	_delay_ms(500);

	//InitTime();

	while(1)
	{
		second = ReadDS1307(DS1307,0x00);
		r = Read_Current_Time(DS1307,CurrentTime,7);
		
		if(r != 0){
			/* Read out current time every 10 seconds,then transmit data through bluetooth */
			_delay_ms(10000);//delay 10 seconds
			//USART0_Send_Byte(0x30 + second/16);
			//USART0_Send_Byte(0x30 + second%16);
			//USART0_Send_Byte(0x0A);//line feed
			USART0_Send_Byte(0x30 + CurrentTime[0]/16);
			USART0_Send_Byte(0x30 + CurrentTime[0]%16);
			USART0_Send_Byte(0x0A);//line feed
			USART0_Send_Byte(0x30 + CurrentTime[1]/16);
			USART0_Send_Byte(0x30 + CurrentTime[1]%16);
			USART0_Send_Byte(0x0A);//line feed
			USART0_Send_Byte(0x30 + CurrentTime[2]/16);
			USART0_Send_Byte(0x30 + CurrentTime[2]%16);
			USART0_Send_Byte(0x0A);//line feed
			USART0_Send_Byte(0x30 + CurrentTime[3]/16);
			USART0_Send_Byte(0x30 + CurrentTime[3]%16);
			USART0_Send_Byte(0x0A);//line feed
			USART0_Send_Byte(0x30 + CurrentTime[4]/16);
			USART0_Send_Byte(0x30 + CurrentTime[4]%16);
			USART0_Send_Byte(0x0A);//line feed
			USART0_Send_Byte(0x30 + CurrentTime[5]/16);
			USART0_Send_Byte(0x30 + CurrentTime[5]%16);
			USART0_Send_Byte(0x0A);//line feed
			USART0_Send_Byte(0x30 + CurrentTime[6]/16);
			USART0_Send_Byte(0x30 + CurrentTime[6]%16);
			USART0_Send_Byte(0x0A);//line feed
			
		}
		_delay_ms(1);
	}
return 0;
}

/*
CPU : ATmega644pA
Frequency : 16MHz

Function : Transmit Received Data from USART

*/
#define F_CPU 16000000UL  /* 16 MHz CPU clock */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile unsigned char rdata;
volatile unsigned char flag = 0;


inline unsigned char setBit(unsigned char d,unsigned char n) {
	return (d | (1<<n));
}
/* Example : data = clearBit(data,1),means clear 1st bit of data(note data holds 1 byte) */
inline unsigned char clearBit(unsigned char d,unsigned char n) {
	return (d & (~(1<<n)));
}

/*
Initialize USART0
*/
void USART0_Init(unsigned int baud)
{
	UCSR0A = 0x00;//defalut value
	UCSR0B = 0x00;//USART Control and Status Register B 		    //¿ØÖÆ¼Ä´æÆ÷ÇåÁã
	UCSR0C = 3<<UCSZ00;//8 bit data
                                                        //Ñ¡ÔñUCSRC£¬Òì²½Ä£Ê½£¬½ûÖ¹                        
                                                     //   Ð£Ñé£¬1Î»Í£Ö¹Î»£¬8Î»Êý¾ÝÎ»
	baud = F_CPU/16/baud - 1;   //²¨ÌØÂÊ×î´óÎª65K
	UBRR0L = baud; 					     	  
	UBRR0H = baud>>8; 		   //ÉèÖÃ²¨ÌØÂÊ
   
	UCSR0B = (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0); //½ÓÊÕ¡¢·¢ËÍÊ¹ÄÜ£¬½ÓÊÕÖÐ¶ÏÊ¹ÄÜ
   
	//SREG = BIT(7);	       //È«¾ÖÖÐ¶Ï¿ª·Å
	DDRD |= 0x02;	           //ÅäÖÃTX0 pin(PD1) ÎªÊä³ö£¨ºÜÖØÒª£©
}
/*
Initialize USART1
*/
void USART1_Init(unsigned int baud)
{
	UCSR1A = 0x00;
	UCSR1B = 0x00;//USART Control and Status Register B 		    //¿ØÖÆ¼Ä´æÆ÷ÇåÁã
	UCSR1C = 3<<UCSZ10;//8 bit data
                                                        //Ñ¡ÔñUCSRC£¬Òì²½Ä£Ê½£¬½ûÖ¹                        
                                                     //   Ð£Ñé£¬1Î»Í£Ö¹Î»£¬8Î»Êý¾ÝÎ»
	baud = F_CPU/16/baud - 1;   //²¨ÌØÂÊ×î´óÎª65K
	UBRR1L = baud; 					     	  
	UBRR1H = baud>>8; 		   //ÉèÖÃ²¨ÌØÂÊ
	
	UCSR1B = (1<<TXEN1)|(1<<RXEN1)|(1<<RXCIE1); //½ÓÊÕ¡¢·¢ËÍÊ¹ÄÜ£¬½ÓÊÕÖÐ¶ÏÊ¹ÄÜ
   	
	//SREG = BIT(7);	       //È«¾ÖÖÐ¶Ï¿ª·Å
	DDRD |= 0x08;	           //ÅäÖÃTX1 pin(PD3) ÎªÊä³ö£¨ºÜÖØÒª£©
}
/*
Send Data Through USART0
*/
inline void UART0_Send_Byte(unsigned char data)
{
	/* waitting for a empty USART Data Register */
	while(!(UCSR0A&(1<<UDRE0))) ;
	UDR0 = data;//USART Data Register
   
	/* waitting for USART Transmit Complete */
	while(!(UCSR0A&(1<<TXC0)));
	UCSR0A |= 1<<TXC0;//set TXC bit manually
}
/*
Send Data Through USART1
*/
inline void UART1_Send_Byte(unsigned char data)
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

	/* Send Received Data to BlueTooth */
	/* waitting for a empty USART Data Register */
	while(!(UCSR0A&(1<<UDRE0))) ;
	UDR0 = temp;//USART Data Register
   
	/* waitting for USART Transmit Complete */
	while(!(UCSR0A&(1<<TXC0)));
	UCSR0A |= 1<<TXC0;//set TXC bit manually

	UCSR1B |= (1<<RXCIE1);//re-enable receiver interrupt(set bit)
}

void initIOforBT() {
    DDRD |= 0x30; //make portd(4:5) output;
    PORTD = clearBit(PORTD,5); //make output 10(connect to Bluetooth);
    PORTD = clearBit(PORTD,4); //make output 10(connect to Bluetooth);
}

void SendDataToZigbee(unsigned char *data,int len){
    int i;
    /* Calculate Check Sum */
    for(i = 0;i < len - 1;++i){
	data[len - 1] += data[i];
    }

    /* Send Data */
    for(i = 0;i < len;++i){
	UART1_Send_Byte(data[i]);
    }
}

void ReadPanId(){
    unsigned char data[] = {0xfc,0x00,0x91,0x03,0xa3,0xb3,0x00};
    int len = sizeof(data);

    SendDataToZigbee(data,len);
}

void SetPanId(int panid){
    unsigned char data[] = {0xfc,0x02,0x91,0x01,0x00,0x00,0x00};
    int len = sizeof(data);

    data[4] = (panid >> 8) & 0xff;
    data[5] = panid & 0xff;
    
    SendDataToZigbee(data,len);
}

void ReadShortAddr(){
    unsigned char data[] = {0xfc,0x00,0x91,0x04,0xc4,0xd4,0x00};
    int len = sizeof(data);

    SendDataToZigbee(data,len);
    /* 0x0000 represent Coordinator */
}

int main()
{
    USART0_Init(38400);//Initialize USART0 with baud rate of 38400
    USART1_Init(38400);//Initialize USART1 with baud rate of 38400
    initIOforBT();

    _delay_ms(10);
    sei();                     //Enable Gloabal Interrupt
    
    //SetPanId(0x5555);
    _delay_ms(2000);
    ReadPanId();
    
    while(1){}

    return 0;

}

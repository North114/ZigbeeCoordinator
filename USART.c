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
//bool flag;
/*
Initialize USART0
*/
void USART0_Init(unsigned int baud)
{
	UCSR0A = 0x00;//defalut value
	UCSR0B = 0x00;//USART Control and Status Register B 		    //���ƼĴ�������
	UCSR0C = 3<<UCSZ00;//8 bit data
                                                        //ѡ��UCSRC���첽ģʽ����ֹ                        
                                                     //   У�飬1λֹͣλ��8λ����λ
	baud = F_CPU/16/baud - 1;   //���������Ϊ65K
	UBRR0L = baud; 					     	  
	UBRR0H = baud>>8; 		   //���ò�����
   
	UCSR0B = (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0); //���ա�����ʹ�ܣ������ж�ʹ��
   
	//SREG = BIT(7);	       //ȫ���жϿ���
	DDRD |= 0x02;	           //����TX0 pin(PD1) Ϊ���������Ҫ��
}
/*
Initialize USART1
*/
void USART1_Init(unsigned int baud)
{
	UCSR1A = 0x00;
	UCSR1B = 0x00;//USART Control and Status Register B 		    //���ƼĴ�������
	UCSR1C = 3<<UCSZ10;//8 bit data
                                                        //ѡ��UCSRC���첽ģʽ����ֹ                        
                                                     //   У�飬1λֹͣλ��8λ����λ
	baud = F_CPU/16/baud - 1;   //���������Ϊ65K
	UBRR1L = baud; 					     	  
	UBRR1H = baud>>8; 		   //���ò�����
	
	UCSR1B = (1<<TXEN1)|(1<<RXEN1)|(1<<RXCIE1); //���ա�����ʹ�ܣ������ж�ʹ��
   	
	//SREG = BIT(7);	       //ȫ���жϿ���
	DDRD |= 0x08;	           //����TX1 pin(PD3) Ϊ���������Ҫ��
}
/*
Send Data Through USART0
*/
void uart_sendB(unsigned char data)
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
void UART1_Send_Byte(unsigned char data)
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

void initIOforBluetooth() {
    DDRD |= 0x30; //make portd(4:5) output;
    PORTD &= 0xCF; //make output 00(connect to bluetooth);
    //PORTD |= 0x30; //make output 0;
}

int main()
{
    USART0_Init(38400);//Initialize USART0 with baud rate of 9600
    USART1_Init(38400);//Initialize USART1 with baud rate of 9600
    initIOforBluetooth();

    _delay_ms(10);
    sei();                     //Enable Gloabal Interrupt

    UART1_Send_Byte(0x36);//for debug use

    while(1)
    {		
    }

    return 0;

}

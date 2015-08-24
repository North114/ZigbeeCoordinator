#define F_CPU 16000000/* 16 MHz CPU clock */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

inline unsigned char setBit(unsigned char d,unsigned char n) {
	return (d | (1<<n));
}
/* Example : data = clearBit(data,1),means clear 1st bit of data(note data holds 1 byte) */
inline unsigned char clearBit(unsigned char d,unsigned char n) {
	return (d & (~(1<<n)));
}

void initSelector() {
    DDRD |= 0x30; //Nakes PORTC as Output
}

int main()
{
    initSelector();
    
    PORTD = setBit(PORTD,4);
    PORTD = setBit(PORTD,5);
    
    return 0;
}

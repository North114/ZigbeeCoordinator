#define F_CPU 16000000/* 16 MHz CPU clock */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void initLED() {
    DDRC |= 0x80; //Nakes PORTC as Output
}

void Blink() {
    PORTC |= 0x80; //Turns ON LEDs
    _delay_ms(1000); //1 second delay
    PORTC &= ~(0x80); //Turns OFF LEDs
    _delay_ms(1000); //1 second delay
}

int main()
{
    initLED();
    while(1) //infinite loop
    {
	Blink();
    }
    return 0;
}

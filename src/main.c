#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

int main (void)
{
    DDRB |= (1 << 5); // Port B configuration

    // Keep MCU alive
    while (1)
    {
        PORTB |= (1 << 5);
        _delay_ms(250);
        PORTB &= ~(1 << 5);
        _delay_ms(250);
    }
    
    return 0;
}
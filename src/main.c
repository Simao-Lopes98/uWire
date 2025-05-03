// main.c

/*

Program starting point

*/

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "common.h"

// Forward declarations
int main (void);
LOCAL void timerSetup (void);


// Design globals
LOCAL int ledToggle = 0;

// Timer 1 ISR
ISR(TIMER1_COMPA_vect)
{
    ledToggle = 1;      // Set flag
}

// Timer 1 A setup
LOCAL void timerSetup (void)
{
    // Reset registers
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // Set Prescaler (1024) and CTC mode
    TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);  // 0x0D

    // Value to compare: 15624 DEC-HEX-> 0x3D08
    OCR1A = 0x3D08;

    // Set the bit 2 of TIMSK1 - Compare Interrupt Enable
    TIMSK1 |= (1 << OCIE1A);

    // Enable ISR
    sei();
}

// Main
int main (void)
{
    DDRB |= (1 << 4); // Port B configuration
    
    timerSetup();
    
    // Keep MCU alive
    while (1)
    {
        if (1 == ledToggle)
        {
            ledToggle = 0;
            PORTB ^= (1 << 4);
        }
        _delay_ms(50);
    }
    
    return 0;
}
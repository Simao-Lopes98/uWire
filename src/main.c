// main.c

/*

Program starting point

*/

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "common.h"
#include "tasks.h"

// Forward declarations
int main (void);
LOCAL void timerSetup (void);


// Design globals
LOCAL int ledToggle = 0;

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
// main.c

/*

Program starting point

*/

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <common.h>
#include <uWire.h>
#include <serial.h>

// Forward declarations
int main (void);
void task1 (void);
void task2 (void);

// Main - Entry point
int main (void)
    {
    // Init LED in IO 13
    DDRB |= (1 << 5);

    // Init Serial
    serial_init(9600);

    /* Init uWire */
    initScheduler();

    wTaskCreate (&task1, "hello", 512);
    wTaskCreate (&task2, "blink", 512);

    while (1)
    {
    _delay_ms (250);
    }
    
    return 0;
    }

void task1 (void)
    {
    printf ("Hello");
    _delay_ms (250);
    }

void task2 (void)
    {
    PORTB |= (1 << 5);
    _delay_ms (500);
    PORTB &= ~(1 << 5);
    _delay_ms (500);
    }
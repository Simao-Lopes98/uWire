// main.c

/*

Program starting point

*/

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <common.h>
#include <tasks.h>
#include <serial.h>

// Forward declarations
int main (void);

// Main - Entry point
int main (void)
    {
    // Init LED in IO 13
    DDRB |= (1 << 5);

    // Init Serial
    serial_init(9600);

    /* Init uWire */
    initScheduler();


    while (1)
    {

    }
    
    return 0;
    }
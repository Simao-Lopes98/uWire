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
LOCAL void blinky1Task (void);
LOCAL void blinky2Task (void);

// Main - Entry point
int main (void)
    {
    
    // Init LED in IO 13
    DDRB |= (1 << 5);
    // Init LED in IO 12
    DDRB |= (1 << 4);

    // Init Serial
    serial_init(9600);

    /* Init uWire */
    initScheduler();

    /* Create tasks */
    wTask_t * blinky2TaskCtrl = wTaskCreate (&blinky2Task, 
                                            "blinky2", 
                                            MINIMAL_STACK_SIZE);
    wTask_t * blinkCtrlBlock = wTaskCreate (&blinky1Task, 
                                            "blink1", 
                                            MINIMAL_STACK_SIZE);

    /* Main loop is used as Idle Task */
    while (1)
        {
        printf ("In Main\n");
        _delay_ms (1000);
        }
    
    return 0;
    }

void blinky1Task (void)
    {

    while (1)
        {
        PORTB |= (1 << 5);
        _delay_ms (50);
        PORTB &= ~(1 << 5);
        _delay_ms (50);
        }
    }

void blinky2Task (void)
    {
    while (1)
        {
        PORTB |= (1 << 4);
        _delay_ms (500);
        PORTB &= ~(1 << 4);
        _delay_ms (500);
        }
    }
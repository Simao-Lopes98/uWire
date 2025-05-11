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
void ledBlink (void);

// Globals
LOCAL Task_t * ledBlinkTask = NULL;

// Main - Entry point
int main (void)
    {
    // Init LED in IO 13
    DDRB |= (1 << 5);

    /* Create Task */
    ledBlinkTask = createTask ("ledBlink", ledBlink, NULL);

    (void) startTask (ledBlinkTask);

    /* Init uWire */
    initScheduler();
    
    return 0;
    }
/*
Test with a simple state-machine.
TODO: Task Stack Management 

*/
void ledBlink (void)
    {
    printf("Entering task\n");
    static int state = 0;
    switch (state)
        {
        case 0:
            PORTB |= (1 << 5);
            taskDelay(500 / TICK_MS);
            state = 1;
            return;
        case 1:
            PORTB &= ~(1 << 5);
            taskDelay(500 / TICK_MS);
            state = 0;
            return;
        default:
            return;
        }    
    }
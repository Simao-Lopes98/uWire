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
LOCAL void bspInit(void);       /* Init BSP */
LOCAL void blinky1Task (void);  
LOCAL void blinky2Task (void);
LOCAL void blinky3Task (void);

// Main - Entry point
int main (void)
    {
    
    bspInit();

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
    wTask_t * blink3CtrlBlock = wTaskCreate (&blinky3Task, 
                                            "blink3", 
                                            MINIMAL_STACK_SIZE);

    /* Main loop is used as Idle Task */
    while (1)
        {
        printf ("In Main\n");
        _delay_ms (1000);
        }
    
    return 0;
    }

LOCAL void blinky1Task (void)
    {
    printf ("Entering Bliny 1\n");
    while (1)
        {
        PORTB |= (1 << 5);
        _delay_ms (50);
        PORTB &= ~(1 << 5);
        _delay_ms (50);
        }
    }

LOCAL void blinky2Task (void)
    {
    printf ("Entering Bliny 2\n");
    while (1)
        {
        PORTB |= (1 << 4);
        _delay_ms (500);
        PORTB &= ~(1 << 4);
        _delay_ms (500);
        }
    }

LOCAL void blinky3Task (void)
    {
    while (1)
        {
        PORTB |= (1 << 3);
        _delay_ms (250);
        PORTB &= ~(1 << 3);
        _delay_ms (250);
        }
    }

LOCAL void bspInit(void)
    {
    // Init LED in IO 13
    DDRB |= (1 << 5);
    // Init LED in IO 12
    DDRB |= (1 << 4);
    // Init LED in IO 11
    DDRB |= (1 << 3);    
    }
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
void blinkTask (void);
void hexDumpStack(wTask_t *task);

// Main - Entry point
int main (void)
    {
    // Init LED in IO 13
    DDRB |= (1 << 5);

    // Init Serial
    serial_init(9600);

    wTask_t * blinkCtrlBlock = wTaskCreate (&blinkTask, "blink", 256);

    hexDumpStack(blinkCtrlBlock);

    /* Init uWire */
    initScheduler();
    
    printf ("Enterying main while\n");

    while (1)
        {
        _delay_ms (250);
        }
    
    return 0;
    }

void hexDumpStack(wTask_t *task)
    {
        UINT8 *sp = (UINT8 *)task->stackPtr;
        printf("\n\n");
        printf("Stack dump from fabricated SP:\n");
        for (uint16_t i = 0; i < task->stackSize; i += 16)
        {
            printf("0x%04X: ", (unsigned)(sp + i));
            for (UINT8 j = 0; j < 16 && (i + j) < task->stackSize; ++j)
            {
                printf("%02X ", sp[i + j]);
            }
            printf("\n");
        }
        UINT16 taskAddr = (UINT16) task->taskFn;
        printf ("Low Byte: %02X. High Byte: %02X\n", 
                (UINT8)(taskAddr & 0xFF), 
                (UINT8)((taskAddr >> 8) & 0xFF));
    }

void blinkTask (void)
    {
    printf ("Starting Task 2\n");

    while (1)
        {
        PORTB |= (1 << 5);
        _delay_ms (500);
        PORTB &= ~(1 << 5);
        _delay_ms (500);
        }
    }
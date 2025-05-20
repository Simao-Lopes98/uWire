/* uWire.c */
/*

Task management lib.
- Manages tasks
- Tick update

*/
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include "common.h"
#include "uWire.h"

/* Forward section */
LOCAL void timerSetup (void);
LOCAL void fillStackContext (wTask_t * taskCtrl);

void TIMER1_COMPA_vect( void ) __attribute__ ( ( signal, naked ) );

/* Globals */

LOCAL wTaskNode_t * taskListHead = NULL; /* Head of Task List */

LOCAL volatile UINT64 tick = 0;  /* Tick counter */

/*******************************************************************************
API Tasks functions
*/
IMPORT void initScheduler(void)
    {

    /* Create tick counter */
    timerSetup();
    }

IMPORT wTask_t * wTaskCreate(wTaskHandler taskFn,
                            const char name[12],
                            UINT16 stackSize)
    {
    wTask_t * taskCtrl = NULL;
    wTaskNode_t * node = NULL;
    wTaskNode_t * tempNode = NULL;
    
    /* Sanity checks */
    if (taskFn == NULL || name == NULL)
        {
        return NULL;
        }

    /* Create task ctrl */
    taskCtrl = (wTask_t *) calloc (1, sizeof (wTask_t));
    if ( taskCtrl == NULL)
        {
        return NULL;
        }
    
    (void) strcpy (taskCtrl->name, name);
    taskCtrl->taskFn = taskFn;
    taskCtrl->stackSize = stackSize;

    taskCtrl->stackPtr = (void *) malloc (stackSize);
    (void) memset (taskCtrl->stackPtr, 0, stackSize);

    /* Fill stack context */
    fillStackContext(taskCtrl);

    /* Create a new node and add to the list */
    node = (wTaskNode_t *) calloc (1, sizeof(wTaskNode_t));
    
    /* Sanity check */
    if (node == NULL)
        {
        return NULL;
        }

    /* Assing to new task node */
    node->task = taskCtrl;
    node->next = NULL;
    
    if (taskListHead == NULL)
        {
        taskListHead = node;
        }
    else
        {
        /* Traverse the list to find the last node */
        tempNode = taskListHead->next;
        while (tempNode != NULL)
            {
            tempNode = tempNode->next;
            }
        tempNode->next = node;
        }

    return taskCtrl;    
    }

LOCAL void fillStackContext (wTask_t * taskCtrl)
    {
    /* taskCtrl already checked on call-tree */

    UINT8 *stack = (UINT8 *) taskCtrl->stackPtr;
    UINT16 addr = (UINT16) &taskCtrl->taskFn; /* entry point of task function */

    /* Move stack pointer to top (stack grows down) */
    stack += taskCtrl->stackSize;

    /* Push return address */
    *(--stack) = (UINT8)(addr & 0xFF);        /* low byte */
    *(--stack) = (UINT8)((addr >> 8) & 0xFF); /* high byte */

    /* Push initial SREG with interrupts enabled (I-bit set) */
    *(--stack) = 0x80;

    /* Push registers R31 to R0 */
    for (int i = 31; i >= 0; i--) {
        *(--stack) = 0x00;
    }

    /* Update the task's stack pointer */
    taskCtrl->stackPtr = (UINT16 *)stack;
    }

/*******************************************************************************
Tick and Context Saving/Restoring managment
*/

ISR (TIMER1_COMPA_vect)
    {
    ++tick;
    }

/*******************************************************************************
Timer Setup for tick counter 
- 10 ms tick period
- OCR1A = (Fcpu.tick)/prescaler - 1 
*/

LOCAL void timerSetup (void)
    {
    /* Disable ISR */
    cli();

    /* Reset registers */
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    /* Set Prescaler (64) and CTC mode */
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);

    /* Value to compare: (16 MHz . 10 ms) / 64 - 1 = 2499 -HEX-> 0x09C3 */
    OCR1A = 0x09C3;

    /* Set the bit 2 of TIMSK1 - Compare Interrupt Enable */
    TIMSK1 |= (1 << OCIE1A);

    /* Enable ISR */
    sei();
    }
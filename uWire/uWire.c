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
#include "log.h"

/* Forward section */
LOCAL void timerSetup (void);
LOCAL void fillStackContext (wTask_t * taskCtrl);

void TIMER1_COMPA_vect( void ) __attribute__ ( ( signal, naked ) );
void wtaskSwitcher ( void );

/* Globals */

volatile UINT64 tick = 0;  /* Tick counter */
wTask_t * volatile wCurrentTask = NULL; /* Save current taks stack */
wTask_t * volatile wNextTask = NULL;    /* Next current taks stack */
LOCAL UINT8 taskNumber = 0;

wTask_t * volatile wTaskList[2];

/*******************************************************************************
API Tasks functions
*/
IMPORT void initScheduler(void)
    {
    
    /* Check if at least one task is created */
    if (wTaskList[0] == NULL)
        {
        CRITICAL_LOG ("ERROR: No task created");
        return;
        }

    /* Set initial task to run */
    wCurrentTask = wTaskList[0];

    /* Create tick counter */
    timerSetup();
    }

IMPORT wTask_t * wTaskCreate(wTaskHandler taskFn,
                            const char name[12],
                            UINT16 stackSize)
    {
    wTask_t * taskCtrl = NULL;

    /* Sanity checks */
    if (taskFn == NULL || name == NULL)
        {
        CRITICAL_LOG("Fail on wTaskCreate - Initial sanity checks");
        return NULL;
        }

    /* Create task ctrl */
    taskCtrl = (wTask_t *) calloc (1, sizeof (wTask_t));
    if ( taskCtrl == NULL)
        {
        CRITICAL_LOG("Fail on wTaskCreate - Fail to allocate heap for task");
        return NULL;
        }
    
    (void) strcpy (taskCtrl->name, name);
    taskCtrl->taskFn = taskFn;
    taskCtrl->stackSize = stackSize;

    taskCtrl->stackPtr = (void *) malloc (stackSize);
    (void) memset (taskCtrl->stackPtr, 0, stackSize);

    /* Fill stack context */
    fillStackContext(taskCtrl);

    // DEBUG
    wTaskList[taskNumber] = taskCtrl;

    /* Update task number */
    taskNumber++;
    return taskCtrl;
    }

LOCAL void fillStackContext (wTask_t * taskCtrl)
    {
    /* taskCtrl already checked on call-tree */

    UINT8 *stack = (UINT8 *) taskCtrl->stackPtr;
    UINT16 addr = (UINT16) taskCtrl->taskFn; /* entry point of task function */

    /* Move stack pointer to top (stack grows down) */
    stack += taskCtrl->stackSize;

    /* This is the SREG that `reti` will pop last */
    *(--stack) = 0x80; // SREG with I-bit (interrupts enabled)

    /* Push return address (PC) */
    *(--stack) = (UINT8)(addr & 0xFF);        /* low byte */
    *(--stack) = (UINT8)((addr >> 8) & 0xFF); /* high byte */

    /* R0 (The original R0 that the ISR would have saved) */
    *(--stack) = 0xDE;

    /* Push initial SREG with interrupts enabled (I-bit set) */
    *(--stack) = 0x80;

    /* Push registers R31 to R0 */
    for (int i = 31; i >= 0; i--) 
        {
        *(--stack) = 0xDE;
        }

    /* Update the task's stack pointer */
    taskCtrl->stackPtr = (UINT16 *)stack;
    }

/*******************************************************************************
Tick and Context Saving/Restoring managment
*/
void wtaskSwitcher ( void )
    {
    /* Will need to be updated for scalability */
    wCurrentTask = (wCurrentTask == wTaskList[0]) ? wTaskList[1] : wTaskList[0];
    }

void TIMER1_COMPA_vect (void)
    {
    /* 
    TODO: Tick has be updated outside, as the aditionall registers will 
    conflict the stack
    */
    // tick++;

    __asm__ __volatile__ (        
        /* --- Save Context --- */
        "push r0               \n\t"              
        "in   r0, __SREG__     \n\t"
        "cli                   \n\t" /* disable interrupts during switch */
        "push r0               \n\t"
        "push r1               \n\t"
        "clr  r1               \n\t"
        "push r2 \n\t push r3 \n\t push r4 \n\t push r5 \n\t"
        "push r6 \n\t push r7 \n\t push r8 \n\t push r9 \n\t"
        "push r10\n\t push r11\n\t push r12\n\t push r13\n\t"
        "push r14\n\t push r15\n\t push r16\n\t push r17\n\t"
        "push r18\n\t push r19\n\t push r20\n\t push r21\n\t"
        "push r22\n\t push r23\n\t push r24\n\t push r25\n\t"
        "push r26\n\t push r27\n\t push r28\n\t push r29\n\t"
        "push r30\n\t push r31\n\t"

        /* Save stack pointer to wCurrentTask->stackPtr */
        "lds  r26, wCurrentTask     \n\t"
        "lds  r27, wCurrentTask+1   \n\t"
        "in   r0, __SP_L__          \n\t"
        "st   x+, r0                \n\t"
        "in   r0, __SP_H__          \n\t"
        "st   x+, r0                \n\t"

        /* Call task switcher */
        "rcall wtaskSwitcher        \n\t"

        /* Restore SP from wCurrentTask->stackPtr */
        "lds  r26, wCurrentTask     \n\t"
        "lds  r27, wCurrentTask+1   \n\t"
        "ld   r28, x+               \n\t"  // _SP_L_
        "out  __SP_L__, r28         \n\t"
        "ld   r29, x+               \n\t"  // _SP_H_
        "out  __SP_H__, r29         \n\t"

        /* Restore Context */
        "pop r31\n\t pop r30\n\t pop r29\n\t pop r28\n\t"
        "pop r27\n\t pop r26\n\t pop r25\n\t pop r24\n\t"
        "pop r23\n\t pop r22\n\t pop r21\n\t pop r20\n\t"
        "pop r19\n\t pop r18\n\t pop r17\n\t pop r16\n\t"
        "pop r15\n\t pop r14\n\t pop r13\n\t pop r12\n\t"
        "pop r11\n\t pop r10\n\t pop r9 \n\t pop r8 \n\t"
        "pop r7 \n\t pop r6 \n\t pop r5 \n\t pop r4 \n\t"
        "pop r3 \n\t pop r2 \n\t pop r1 \n\t"
        "pop r0                     \n\t"
        "out __SREG__, r0           \n\t"
        "pop r0                     \n\t"
        "reti                       \n\t"               /* Exit ISR */
        );
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
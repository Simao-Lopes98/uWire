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
LOCAL void idleTask(void);
LOCAL void saveContext (void);
LOCAL void incTick (void);
LOCAL void switchContext (void);
LOCAL void restoreContext (void);
LOCAL void fillStackContext (Task_t * taskCtrl);

/* Globals */

LOCAL TaskNode_t * taskListHead = NULL; /* Head of Task List */

LOCAL volatile UINT16 * currentTaskContext = NULL; /* Task Saving context */

LOCAL volatile UINT64 tick = 0;  /* Tick counter */

void TIMER1_COMPA_vect ( void ) __attribute__ ((signal, naked));

LOCAL void idleTask(void)
    {
    /* Idle task does nothing. Add a nop to avoid any watchdog trigger */
    __asm__ __volatile__ ("nop");
    return;
    }

/*******************************************************************************
API Tasks functions
*/
IMPORT void initScheduler(void)
    {
    STATUS ret = ERROR;
    Task_t * taskIdle = NULL;
    
    /* createTask */
    taskIdle = createTask (idleTask , NULL, "idle", MINIMAL_STACK_SIZE, 0);
    if (taskIdle == NULL)
        {
        /* TODO: Add Critical ERROR LOG */
        return;
        }
    
    /* Init task list */
    taskListHead = (TaskNode_t *) calloc (1, sizeof(TaskNode_t));
    if (taskListHead == NULL)
        {
        /* TODO: Add Critical ERROR LOG */
        return;
        }
    
    taskListHead->task = taskIdle;
    taskListHead->next = NULL;
    
    /* Start idleTask */
    ret = startTask (taskIdle);
    if (ret == ERROR)
        {
        /* TODO: Add Critical ERROR LOG */
        return;
        }
    
    /* Set the currentTaskContext to taskIdle */
    currentTaskContext = taskIdle->stackPtr;

    /* Create tick counter */
    timerSetup();
    }

IMPORT Task_t * createTask( void (* taskRoutine), 
                            void *arguments,
                            const char name[12],
                            UINT16 stackSize,
                            UINT8 prio)
    {
    Task_t * taskCtrl = NULL;
    TaskNode_t * node = NULL;
    TaskNode_t * tempNode = NULL;
    
    /* Sanity checks */
    if (name == NULL)
        {
        return NULL;
        }
    
    if (taskRoutine == NULL)
        {
        return NULL;
        }

    taskCtrl = (Task_t *) calloc (1, sizeof (Task_t));
    if ( taskCtrl == NULL)
        {
        return NULL;
        }
    
    (void) strcpy (taskCtrl->name, name);
    taskCtrl->task = taskRoutine;
    taskCtrl->argument = arguments;
    taskCtrl->status = STOPPED;
    taskCtrl->stackSize = stackSize;

    taskCtrl->stackPtr = (UINT16 *) malloc (stackSize);
    (void) memset (taskCtrl->stackPtr, 0, stackSize);

    fillStackContext(taskCtrl);

    /* Create a new node and add to the list */
    node = (TaskNode_t *) calloc (1, sizeof(TaskNode_t));
    
    /* Sanity check */
    if (node == NULL || taskListHead == NULL)
        {
        return NULL;
        }

    /* Assing to new task node */
    node->task = taskCtrl;
    node->next = NULL;

    /* Traverse the list to find the last node */
    tempNode = taskListHead->next;
    while (tempNode != NULL)
        {
        tempNode = tempNode->next;
        }
    tempNode->next = node;
    
    return taskCtrl;
    }

LOCAL void fillStackContext (Task_t * taskCtrl)
    {
    /* taskCtrl already checked on call-tree */

    UINT8 *stack = (UINT8 *) taskCtrl->stackPtr;
    UINT16 addr = (UINT16) taskCtrl->task; /* entry point of task function */

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

IMPORT STATUS startTask (Task_t * taskCtrl)
    {
    if (taskCtrl == NULL)
        {
        return ERROR;
        }
    
    taskCtrl->status = RUNNING;
    
    return OK;
    }

/*******************************************************************************
Tick and Context Saving/Restoring managment
*/

ISR (TIMER1_COMPA_vect)
    {

    /* Save context */
    // saveContext();

    /* 
    Increment tick count and check if the new tick value has caused a delay 
    period to expire 
    */
    // incTick();
    tick++;

    /* 
    Verify if a context switch is required.
    Switch to the context of the task made ready to run by incTick()
    */
    // switchContext ();

    /*
    Restore the context. If a context switch has occurend this will restore 
    the context of the interrupted task.
    */
    // restoreContext();


    /* Return from ISR */
    __asm__ __volatile__ ("reti");
    }

LOCAL void saveContext ( void )
    {
    /*__asm__ __volatile__ (
        "push r0 "
        "in r0, SREG"
        "cli"
        "push r0"
        "push r1"
        "clr r1"
        "push r2"
        "push r3"
        "push r4"
        "push r5"
        "push r6"
        "push r7"
        "push r8"
        "push r9"
        "push r10"
        "push r11"
        "push r12"
        "push r13"
        "push r14"
        "push r15"
        "push r16"
        "push r17"
        "push r18"
        "push r19"
        "push r20"
        "push r21"
        "push r22"
        "push r23"
        "push r24"
        "push r25"
        "push r26"
        "push r27"
        "push r28"
        "push r29"
        "push r30"
        "push r31"
        "lds r26, currentTaskContext"
        "lds r27, currentTaskContext + 1"
        "in r0, SPL"
        "st x+, r0 "
        "in r0, SPH"
        "st x+, r0 "
        );*/
    }

LOCAL void incTick (void)
    {
    
    }

LOCAL void switchContext (void)
    {
    
    }

LOCAL void restoreContext (void)
    {
    
    }

/*
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
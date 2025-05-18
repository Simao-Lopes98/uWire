/* tasks.c */
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
#include "tasks.h"

/* Forward section */
LOCAL void runScheduler(void);
LOCAL void timerSetup (void);
LOCAL void idleTask(void);
LOCAL void saveContext (void);
LOCAL void incTick (void);
LOCAL void switchContext (void);
LOCAL void restoreContext (void);

/* Globals */

/* TODO: Turn into linked list */
LOCAL Task_t * taskTable [TASK_TABLE_SIZE] = {NULL, NULL, NULL};

LOCAL volatile UINT64 tick = 0;  /* Tick counter */
LOCAL UINT8 taskIndex = 0; /* Index to the task table */

void TIMER1_COMPA_vect ( void ) __attribute__ ((signal, naked));

/*******************************************************************************
Scheduler helper functions
*/
LOCAL void idleTask(void)
    {
    /* Idle task does nothing. Add a nop to avoid any watchdog trigger */
    __asm__ __volatile__ ("nop");
    return;
    }

LOCAL void runScheduler(void)
    {
    
    UINT64 oldTick = 1;
    Task_t * taskCtrl = NULL;

    /* Scheduler */
    while (1)
        {
        if (tick != oldTick)
            {
            oldTick = tick;
            taskCtrl = taskTable [taskIndex];

            if (taskCtrl == NULL)
                {
                taskIndex++;
                if (taskIndex == TASK_TABLE_SIZE)
                    {
                    taskIndex = 0;
                    }
                continue;
                }
            if (taskCtrl->status == STOPPED)
                {
                taskIndex++;
                if (taskIndex == TASK_TABLE_SIZE)
                    {
                    taskIndex = 0;
                    }
                continue;
                }
            if (taskCtrl->status == PENDED)
                {
                taskCtrl->ticks--; // Decrement ticks
                if (taskCtrl->ticks == 0)
                    {
                    taskCtrl->status = RUNNING;
                    }
                taskIndex++;
                if (taskIndex == TASK_TABLE_SIZE)
                    {
                    taskIndex = 0;
                    }
                continue;
                }

            /* Invoke task routine */
            taskCtrl->task (taskCtrl->argument);

            taskIndex++;
            if (taskIndex == TASK_TABLE_SIZE)
                {
                taskIndex = 0;
                }
            }
        }
    }

/*******************************************************************************
API Tasks functions
*/
IMPORT void initScheduler(void)
    {
    STATUS ret = ERROR;
    Task_t * taskIdle = NULL;
    
    /* Create tick counter */
    timerSetup();
    
    /* createTask */
    taskIdle = createTask ("idle", idleTask , NULL);
    if (taskIdle == NULL)
        {
        /* TODO: Add Critical ERROR LOG */
        return;
        }
    
    /* Start idleTask */
    ret = startTask (taskIdle);
    if (ret == ERROR)
        {
        /* TODO: Add Critical ERROR LOG */
        return;
        }
    
    runScheduler();
    }

IMPORT Task_t * createTask( const char name[12],
                            void (* taskRoutine), 
                            void *arguments)
    {
    Task_t * taskCtrl = NULL;
    
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

    /* Check if table is FULL 
    
    TODO: Table should not be static

    */

    if (taskTable[TASK_TABLE_SIZE - 1] != NULL)
        {
        free (taskCtrl);
        return NULL;
        }
    
    /* Assing to the task table 
    
    Find the 1st available space
    
    */
    for (size_t i = 0; i < TASK_TABLE_SIZE; i++)
        {
        if (taskTable[i] == NULL)
            {
            taskCtrl->id = (UINT8) i;
            taskTable[i] = taskCtrl;
            break;
            }
        }
    
    return taskCtrl;
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

IMPORT Task_t * getCurrentTask (void)
    {
    return taskTable [taskIndex];
    }

IMPORT void taskDelay (UINT32 ticks)
    {
    Task_t * currentTask = NULL;
    
    currentTask = getCurrentTask ();
    if (currentTask == NULL)
        {
        return;
        }
    currentTask->status = PENDED;
    currentTask->ticks = ticks; /* ticks to be counted */
    }

/*******************************************************************************
Tick and Context Saving/Restoring managment
*/

ISR (TIMER1_COMPA_vect)
    {

    /* Save context */
    saveContext();

    /* 
    Increment tick count and check if the new tick value has caused a delay 
    period to expire 
    */
    incTick();

    /* 
    Verify if a context switch is required.
    Switch to the context of the task made ready to run by incTick()
    */
    switchContext ();

    /*
    Restore the context. If a context switch has occurend this will restore 
    the context of the interrupted task.
    */
    restoreContext();


    /* Return from ISR */
    __asm__ __volatile__ ("reti");
    }

LOCAL void saveContext ( void )
    {
    __asm__ __volatile__ (
    "push r0 nt"
    "in r0, __SREG__ nt"
    "cli nt"
    "push r0 nt"
    "push r1 nt"
    "clr r1 nt"
    "push r2 nt"
    "push r3 nt"
    "push r4 nt"
    "push r5 nt"
    "push r6 nt"
    "push r7 nt"
    "push r8 nt"
    "push r9 nt"
    "push r10 nt"
    "push r11 nt"
    "push r12 nt"
    "push r13 nt"
    "push r14 nt"
    "push r15 nt"
    "push r16 nt"
    "push r17 nt"
    "push r18 nt"
    "push r19 nt"
    "push r20 nt"
    "push r21 nt"
    "push r22 nt"
    "push r23 nt"
    "push r24 nt"
    "push r25 nt"
    "push r26 nt"
    "push r27 nt"
    "push r28 nt"
    "push r29 nt"
    "push r30 nt"
    "push r31 nt"
    "lds r26, pxCurrentTCB nt"
    "lds r27, pxCurrentTCB + 1 nt"
    "in r0, __SP_L__ nt"
    "st x+, r0 nt"
    "in r0, __SP_H__ nt"
    "st x+, r0 nt"
    );
    }

LOCAL void incTick (void)
    {
    tick++;
    }

LOCAL void switchContext (void)
    {
    ...
    }

LOCAL void restoreContext (void)
    {
    ...
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
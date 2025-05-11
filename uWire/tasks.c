/* tasks.c */

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

/* Globals */

/* TODO: Turn into linked list */
LOCAL Task_t * taskTable [TASK_TABLE_SIZE] = {NULL, NULL, NULL};

LOCAL volatile UINT64 tick = 0;  /* Tick counter */
LOCAL UINT8 taskIndex = 0; /* Index to the task table */

/*******************************************************************************
Main scheduler for the uWire
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

IMPORT Task_t * createTask(const char name[12],
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

LOCAL void idleTask(void)
    {
    /* TODO: Add some more processing */
    __asm__ __volatile__ ("NOP");
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
Task Helpers
*/

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

/* Tick Managment */

ISR(TIMER1_COMPA_vect)
    {
    tick++;
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
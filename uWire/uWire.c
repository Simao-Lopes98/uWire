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
LOCAL void timer1Setup (void);
LOCAL void timer2Setup (void);
LOCAL void fillStackContext (wTask_t * taskCtrl);
LOCAL wTask_t * createMainTask (void);
LOCAL wTask_t * createIdleTask (wTaskHandler taskFn, UINT16 stackSize);
LOCAL STATUS insertTaskNode (wTask_t * taskCtrl);
LOCAL void wTaskYield(void);
LOCAL void idleTask (void);

void TIMER1_COMPA_vect(void) __attribute__ ( ( signal, naked ) );
void TIMER2_COMPA_vect(void) __attribute__ ( ( signal, naked ) );
void wtaskSwitcher (void);
void wTickManagment (void);
void disableTimer2 (void);

/* Globals */

wTask_t * volatile wCurrentTask = NULL; /* Save current taks stack */
wTaskNode_t * volatile taskHeadNode = NULL; /* List of tasks TCB */
wTask_t * volatile wIdleTask = NULL; /* Idle task for scheduller */

/*******************************************************************************
* API Tasks functions
*/

/* Init uWire scheduler - Creates idle and main tasks - Starts tick timer */
IMPORT void initScheduler(void)
    {
    wTask_t * mainTaskCtrl = NULL;

    /* Create idle task */
    wIdleTask = createIdleTask (&idleTask, IDLE_TASK_STACK);
    if (wIdleTask == NULL)
        {
        CRITICAL_LOG ("Fail creating task for idle");
        return;        
        }

    mainTaskCtrl = createMainTask();
    if (mainTaskCtrl == NULL)
        {
        CRITICAL_LOG ("Fail creating task for main");
        return;
        }

    /* Set the current 1st current task to main */
    wCurrentTask = mainTaskCtrl;

    /* Setup tick ISR */
    timerSetup();
    }

/* Creates tasks */
IMPORT wTask_t * wTaskCreate(wTaskHandler taskFn,
                            const char name[12],
                            UINT16 stackSize)
    {
    wTask_t * taskCtrl = NULL;
    
    /* Disable ISR */
    cli();

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
    taskCtrl->taskStatus = TASK_RUNNING;

    taskCtrl->stackPtr = (void *) malloc (stackSize);
    if (taskCtrl->stackPtr == NULL)
        {
        CRITICAL_LOG("Fail on wTaskCreate - Fail to "
                    "allocate heap for task stack");
        free (taskCtrl);
        return NULL;        
        }
    (void) memset (taskCtrl->stackPtr, 0, stackSize);

    /* Fill stack context */
    fillStackContext(taskCtrl);

    if (insertTaskNode (taskCtrl) != OK)
        {
        CRITICAL_LOG("Fail on wTaskCreate - Fail to add task to the list");
        free (taskCtrl->stackPtr);
        free (taskCtrl);
        return NULL;
        }

    /* Enable ISR */
    sei();

    return taskCtrl;
    }

IMPORT void hexDumpStack(wTask_t *task)
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

IMPORT STATUS wTaskDelay(UINT64 ticks)
    {
    if (ticks == 0U || wCurrentTask == NULL)
        {
        return ERROR;
        }

    /* Set to number of ticks to wait */
    wCurrentTask->ticksToDelay = ticks;
    
    /* Set the task to STOPPED */
    wCurrentTask->taskStatus = TASK_STOPPED;
    
    /* Trigger context switch */
    wTaskYield();

    return OK;
    }

IMPORT wTask_t * acquireTaskByName(const char * taskName)
    {
    wTaskNode_t * node = taskHeadNode;

    /* Iterate each task */
    while (node != NULL)
        {
        wTask_t * task = node->task;

        if (strncmp(taskName, task->name, 12) == 0)
            {
            return task; /* Task Found */
            }
        node = node->next;
        }
    
    return NULL; /* No task Found */
    }

/*******************************************************************************
* Private Tasks functions
*/

/* Yield task - Calls the tick ISR straight up */
LOCAL void wTaskYield(void)
    {
    cli();  /* Disable ISR */

    /* Set counter just before compare match */
    TCNT2 = YIELD_ISR_TO_COMPARE - 1;
    
    /* Enable interrupt */
    TIMSK2 |= (1 << OCIE2A);
    
    sei();  /* Enable ISR */
    }

/* Context filling routine */
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

/* Task for Main setup */
LOCAL wTask_t * createMainTask (void)
    {
    wTask_t * taskCtrl = NULL;

    /* Create TCB for main */
    taskCtrl = (wTask_t *) calloc (1, sizeof (wTask_t));
    if ( taskCtrl == NULL)
        {
        return NULL;
        }
    
    (void) strcpy (taskCtrl->name, "main");
    /* No need to setup the function ptr */
    taskCtrl->stackSize = MINIMAL_STACK_SIZE;
    taskCtrl->taskStatus = TASK_RUNNING;
    taskCtrl->stackPtr = (void *) malloc (MINIMAL_STACK_SIZE);
    (void) memset (taskCtrl->stackPtr, 0, MINIMAL_STACK_SIZE);

    if (insertTaskNode (taskCtrl) != OK)
        {
        CRITICAL_LOG ("Fail adding main task to the list");
        return NULL;
        }
    
    return taskCtrl;
    }

/* Idle task create - Is not added to the task list */
LOCAL wTask_t * createIdleTask (wTaskHandler taskFn, UINT16 stackSize)
    {
    wTask_t * taskCtrl = NULL;
    
    /* Disable ISR */
    cli();

    /* Sanity checks */
    if (taskFn == NULL)
        {
        CRITICAL_LOG("Fail creating Idle task - taskFn is NULL");
        return NULL;
        }

    /* Create task ctrl */
    taskCtrl = (wTask_t *) calloc (1, sizeof (wTask_t));
    if ( taskCtrl == NULL)
        {
        CRITICAL_LOG("Fail creating Idle task - Fail allocating TCB");
        return NULL;
        }
    
    (void) strcpy (taskCtrl->name, "idle");
    taskCtrl->taskFn = taskFn;
    taskCtrl->stackSize = stackSize;
    taskCtrl->taskStatus = TASK_RUNNING;

    taskCtrl->stackPtr = (void *) malloc (stackSize);
    if (taskCtrl->stackPtr == NULL)
        {
        CRITICAL_LOG("Fail creating Idle task - Fail to "
                    "allocate heap for task stack");
        free (taskCtrl);
        return NULL;        
        }
    (void) memset (taskCtrl->stackPtr, 0, stackSize);

    /* Fill stack context */
    fillStackContext(taskCtrl);

    /* Do not insert it on the task list */

    /* Enable ISR */
    sei();

    return taskCtrl;    
    }

/* Insert a task node at the end of the list */
LOCAL STATUS insertTaskNode (wTask_t * taskCtrl)
    {
    wTaskNode_t * taskNode = NULL;
    wTaskNode_t * currentNode = NULL;

    taskNode = (wTaskNode_t *) calloc (1, sizeof(wTaskNode_t));
    if (taskNode == NULL)
        {
        return ERROR;
        }
    
    /* taskCtrl was verified in the call-tree */
    taskNode->task = taskCtrl;
    taskNode->next = NULL;

     
    if (taskHeadNode == NULL)
        {
        /* First node in the list */
        taskHeadNode = taskNode;
        return OK;
        }

    /* Inserts in the end of the SLL */
    currentNode = taskHeadNode;
    while (currentNode->next != NULL)
        {
        currentNode = currentNode->next;
        }

    currentNode->next = taskNode;
    return OK;
    }

/* Idle Task - Default task for the scheduler - Has to be always RUNNING */
__attribute__((optimize("O0"))) /* Do not allow compiler optimization */
LOCAL void idleTask (void)
    {
    while (1)
        {
        __asm__ __volatile__ ("nop \n\t"); /* Perform NOP */
        }
    }

/*******************************************************************************
* Tick and Context Saving/Restoring Management
*/

/* Tick management routine - Decrements ticks from STOPPED tasks */
void wTickManagment (void)
    {
    wTaskNode_t * node = taskHeadNode;

    /* Iterate each task */
    while (node != NULL)
        {
        wTask_t * task = node->task;

        if (task->taskStatus == TASK_STOPPED && task->ticksToDelay >= 0)
            {
            if (task->ticksToDelay == 0)
                {
                /* Mark the task as RUNNING */
                task->taskStatus = TASK_RUNNING;
                }
            else
                {
                /* Decrement ticks to delay */
                task->ticksToDelay--;
                }
                
            }
        node = node->next;
        }

    }

/* Task Switcher - Routine goes through the task list */
void wtaskSwitcher (void)
    {    
    static wTaskNode_t * currentNode = NULL;
    wTaskNode_t * startNode = NULL;

    /* Point to the head node as the main task is the 1st to run */
    if (currentNode == NULL)
        {
        currentNode = taskHeadNode;
        }

    /* Start from next task in round-robin order */
    currentNode = (currentNode->next != NULL) ? 
                currentNode->next : taskHeadNode;
    
    startNode = currentNode;  // Remember where we started

    do 
    {
        wTask_t * task = currentNode->task;
        if (task->taskStatus == TASK_RUNNING)
            {
            wCurrentTask = task;
            return;
            }

        currentNode = 
                (currentNode->next != NULL) ? currentNode->next : taskHeadNode;
    } while (currentNode != startNode);  /* Stop if it looped around */

    /* No RUNNING task found — fallback to idle or first task */
    wCurrentTask = wIdleTask;
    }

/* Disable Timer 2 aux routine */
void disableTimer2 (void)
    {
    TIMSK2 &= ~(1 << OCIE2A);
    }

/* Tick ISR */
void TIMER1_COMPA_vect (void)
    {

    __asm__ __volatile__ (
        /* --- Save Context --- */
        "push r0                \n\t"              
        "in   r0, __SREG__      \n\t"
        "cli                    \n\t" /* disable interrupts during switch */
        "push r0                \n\t"
        "push r1                \n\t"
        "clr  r1                \n\t"
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

        /* Call Tick Management */
        "rcall wTickManagment       \n\t"

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

/* Yield ISR */
void TIMER2_COMPA_vect (void)
    {

    __asm__ __volatile__ (
        /* --- Save Context --- */
        "push r0                \n\t"              
        "in   r0, __SREG__      \n\t"
        "cli                    \n\t" /* disable interrupts during switch */
        "push r0                \n\t"
        "push r1                \n\t"
        "clr  r1                \n\t"
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

        /* Disable Timer2 */
        "rcall disableTimer2        \n\t"
        
        /* Call Tick Management */
        "rcall wTickManagment       \n\t"

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
* Timer Setup for tick counter 
* 10 ms tick period
* OCR1A = (Fcpu.tick)/prescaler - 1 
*/

LOCAL void timerSetup (void)
    {
    /* Disable ISR */
    cli();

    /* Tick Timer Setup */
    timer1Setup();

    /* Yield ISR Setup */
    timer2Setup();

    /* Enable ISR */
    sei();
    }

/* Timer for tick - 10 ms period */
LOCAL void timer1Setup (void)
    {
    /* Reset registers */
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;

    /* Set Prescaler (64) and CTC mode */
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);

    /* Set value to compare: (16 MHz . 10 ms) / 64 - 1 = 2499 -HEX-> 0x09C3 */
    OCR1A = TICK_ISR_TO_COMPARE;

    /* Set the bit 2 of TIMSK1 - Compare Interrupt Enable */
    TIMSK1 |= (1 << OCIE1A);   
    }

/* Software timer to handle yield */
LOCAL void timer2Setup (void)
    {
    
    /* Reset registers */
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2  = 0;
    
    /* CTC mode */
    TCCR2A |= (1 << WGM21);
    
    /* Set Prescaler (64) and CTC mode  */
    TCCR2B |= (1 << CS22);

    OCR2A  = YIELD_ISR_TO_COMPARE;

    /* Disable ISR - Should only be enabled by yield function */
    TIMSK2 &= ~(1 << OCIE2A);
    }
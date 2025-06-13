/* uWire.h */

#ifndef UWIRE_H
#define UWIRE_H

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "common.h"
#include "uWire.h"

#define TICK_MS    10           // 1 tick = 10 milliseconds
#define MINIMAL_STACK_SIZE 256  // Minimal stack size

/* Set value to compare: (16 MHz . 10 ms) / 64 - 1 = 2499 -HEX-> 0x09C3 */
#define TICK_ISR_TO_COMPARE 0x09C3

/* typedefs */

/* Task Function pointer */
typedef void (* wTaskHandler) ();

/* Task Status enum */
typedef enum
    {
    TASK_RUNNING,
    TASK_STOPPED,
    TASK_STATUS_END_ENUM
    } wTaskStatus_t;

/* Task Control Block */
typedef struct task
    {
    void * stackPtr;                /* HAS TO BE 1st! Task Stack pointer */
    UINT16 stackSize;               /* Task Stack Size*/
    char name [12];                 /* Task Name */
    wTaskHandler taskFn;            /* Task routine */
    wTaskStatus_t taskStatus;       /* Task Status */
    UINT64 ticksToDelay;            /* Ticks to Pend the Task */
    } wTask_t;

/* Task Node for task list */
typedef struct taskNode
    {
    wTask_t * task;
    struct taskNode * next;
    } wTaskNode_t;

/* Forward section */

IMPORT void initScheduler(void);
IMPORT wTask_t * wTaskCreate(wTaskHandler taskFn,
                            const char name[12],
                            UINT16 stackSize);
IMPORT void hexDumpStack(wTask_t *task);
IMPORT STATUS wTaskDelay(UINT64 ticks);

#endif /* UWIRE_H */
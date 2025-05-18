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

/* typedefs */

/* Enum */
typedef enum taskStatus_t
    {
    RUNNING = 0,
    PENDED,
    STOPPED,
    END_STATUS
    }taskStatus;

typedef struct task
    {
    UINT16 * stackPtr;              /* HAS TO BE 1st! Task Stack pointer */
    UINT16 stackSize;               /* Task Stack Size*/
    char name [12];                 /* Task Name */
    void (*task) (void *argument);  /* Task routine */
    void * argument;                /* Routine arguments */
    taskStatus status;              /* Task status */
    UINT8 prio;                     /* TODO: Priority */
    UINT8 id;
    } Task_t;

/* Task Node for task list */
typedef struct taskNode
    {
    Task_t * task;
    struct taskNode * next;
    } TaskNode_t;

/* Forward section */
IMPORT void initScheduler(void);
IMPORT Task_t * createTask( void (* taskRoutine), 
                            void *arguments,
                            const char name[12],
                            UINT16 stackSize,
                            UINT8 prio);
IMPORT STATUS startTask (Task_t * taskCtrl);



#endif /* UWIRE_H */
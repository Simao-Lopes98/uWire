/* tasks.h */

#ifndef TASKS_H
#define TASKS_H

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "common.h"
#include "tasks.h"

#define TASK_TABLE_SIZE 3

#define TICK_MS    10 // 1 tick = 10 milliseconds

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
    char name [12];                 /* Task Name */
    void (*task) (void *argument);  /* Task routine */
    void * argument;                /* Routine arguments */
    taskStatus status;              /* Task status */
    UINT8 prio;                     /* TODO: Priority */
    UINT8 id;
    UINT32 ticks;
    } Task_t;

/* Forward section */
IMPORT void initScheduler(void);
IMPORT Task_t * createTask( const char name[12],
                            void (*taskRoutine), 
                            void *arguments);
IMPORT STATUS startTask (Task_t * taskCtrl);
IMPORT Task_t * getCurrentTask (void);
IMPORT void taskDelay (UINT32 ticks);



#endif /* TASKS_H */
# uWire
Simple RTOS

## Phase 1 Goals
Create a bare-metal task scheduler where each task runs cooperatively—meaning the task voluntarily returns control to the scheduler (no preemption).

* Run multiple "tasks" (functions) pseudo-concurrently.
* Use a simple round-robin algorithm.
* Track delays or states using software timers or flags.
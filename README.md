# uWire
Simple RTOS


## Phase 1 Goals
Create a bare-metal task scheduler where each task runs cooperatively—meaning the task voluntarily returns control to the scheduler (no preemption).

* Run multiple "tasks" (functions) pseudo-concurrently. ✔
* Use a simple round-robin algorithm. ✔
* Track delays or states using software timers or flags. ✔

## Phase 2 Goals
* Implement Context Switching ✔
* Save and restore CPU state (registers + stack pointer) between tasks. ✔
* Tasks no longer return to the scheduler; instead, they're interrupted and resumed later. ✔

## Phase 3 Goals
* Add Idle Task ✔
* Code hardening API functions ✔
* Implement a task list using a SLL, instead of an array of TCB ✔
* Update tick management ✔
* Create task status ✔
* Create a task delay based on ticks ✔

# Make Commands

To compile
```` Bash
make
````

To flash
```` Bash
make flash
````

Dump elf. file
```` Bash
make dump
````

To clean .o, .elf, .hex files
```` Bash
make clean
````

## WSL Link ATMega328
Be sure to follow [Microsoft - WSL - Connect USB](https://learn.microsoft.com/en-us/windows/wsl/connect-usb) to share USB between host and WSL

Windows CMD
```` PowerShell
usbipd list
````

```` PowerShell
usbipd attach --wsl --busid <busid>
````

Linux Terminal
```` Bash
lsusb
````

## Using Minicom

Install Minicom
```` Bash
sudo apt-get install minicom
````

Opening Serial Monitor
```` Bash
minicom -D /dev/ttyACM<N> -b <BAUD_RATE>
````

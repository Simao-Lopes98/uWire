# uWire
Simple RTOS

# Make Commands

To compile
```` Bash
make
````

To flash
```` Bash
make flash
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

## Phase 1 Goals
Create a bare-metal task scheduler where each task runs cooperatively—meaning the task voluntarily returns control to the scheduler (no preemption).

* Run multiple "tasks" (functions) pseudo-concurrently.
* Use a simple round-robin algorithm.
* Track delays or states using software timers or flags.
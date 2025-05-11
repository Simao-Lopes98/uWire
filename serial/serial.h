/* serial.h */

#ifndef SERIAL_H
#define SERIAL_H

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "common.h"
#include "serial.h"

#define MYUBRR F_CPU/16/BAUD-1

IMPORT void serial_init(UINT32 baud);

#endif /* SERIAL_H */
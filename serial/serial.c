/* serial. c */

#include "serial.h"

LOCAL void uart_init(unsigned int ubrr);
LOCAL void uart_putchar(char c);
LOCAL int uart_putc(char c, FILE *stream);

FILE uart_stdout = FDEV_SETUP_STREAM(uart_putc, NULL, _FDEV_SETUP_WRITE);

LOCAL void uart_init(unsigned int ubrr) {
    // Set baud rate
    UBRR0H = (unsigned char)(ubrr>>8);
    UBRR0L = (unsigned char)ubrr;
    
    // Enable transmitter
    UCSR0B = (1<<TXEN0);
    
    // Set frame format: 8 data bits, 1 stop bit
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

LOCAL void uart_putchar(char c) {
    // Wait for empty transmit buffer
    while (!(UCSR0A & (1<<UDRE0)));
    // Put data into buffer
    UDR0 = c;
}

LOCAL int uart_putc(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r'); // For newline compatibility
    uart_putchar(c);
    return 0;
}

IMPORT void serial_init(UINT32 baud) {
    uart_init(16000000/(16 * baud) -1);
    stdout = &uart_stdout; // Redirect stdout
}
#include "smart_mall.h"

void uart_init(void) {
    // Configure UART for serial communication
    UBRR0H = (uint8_t) (BAUD_PRESCALER >> 8);
    UBRR0L = (uint8_t) BAUD_PRESCALER;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1 format
}

void uart_transmit(unsigned char data) {
    //    wait for empty buffer
    while (!(UCSR0A & (1 << UDRE0)));
    //    then put data in buffer
    UDR0 = data;
}

void uart_send_string(char *str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

unsigned char uart_receive(void) {
    //    poll for data
    while (!(UCSR0A & (1 << RXC0)));
    //    get the data
    return UDR0;
}


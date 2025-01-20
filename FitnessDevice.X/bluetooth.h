#ifndef F_CPU
#define F_CPU 3333333
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>

#define BUF_SIZE 128
#define BLE_RADIO_PROMPT "CMD> "
#define SAMPLES_PER_BIT 16
#define USART_BAUD_VALUE(BAUD_RATE) (uint16_t) ((F_CPU << 6) / (((float) SAMPLES_PER_BIT) * (BAUD_RATE)) + 0.5)


#ifndef BLUETOOTH_H
#define	BLUETOOTH_H

void USART_init();
void usartWriteChar(char c);
void usartWriteCommand(const char *cmd);
char usartReadChar();
void usartReadUntil(char *dest, const char *end_str);
void BLE_init(const char *name);
void BLE_send_data(uint16_t *data, size_t length);

#endif	/* BLUETOOTH_H */

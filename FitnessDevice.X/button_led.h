#ifndef F_CPU
#define F_CPU 3333333
#endif

#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>

#ifndef BUTTON_LED_H
#define	BUTTON_LED_H

#define RED_BUTTON_INTERRUPT (PORTA.INTFLAGS & PIN6_bm)
#define RED_BUTTON_INTERRUPT_CLEAR (PORTA.INTFLAGS = PIN6_bm)
#define YELLOW_BUTTON_INTERRUPT (PORTA.INTFLAGS & PIN4_bm)
#define YELLOW_BUTTON_INTERRUPT_CLEAR (PORTA.INTFLAGS = PIN4_bm)

void button_init();
void LED_init();
void set_LED_color(uint8_t red, uint8_t green, uint8_t blue);

#endif	/* BUTTON_LED_H */


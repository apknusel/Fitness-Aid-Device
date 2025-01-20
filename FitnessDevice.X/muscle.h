#ifndef F_CPU
#define F_CPU 3333333
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>

#ifndef MUSCLE_H
#define	MUSCLE_H

void ADC_init();
uint16_t ADC_read();

#endif	/* MUSCLE_H */


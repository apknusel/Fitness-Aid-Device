#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "muscle.h"
#include "max30102.h"
#include "bluetooth.h"
#include "button_led.h"
#include "max30102_math.h"

#ifndef NEWAVIR_MAIN_H
#define	NEWAVIR_MAIN_H

// Definitions for keep track of time
extern volatile uint32_t milliseconds;
#define millis() (milliseconds + RTC.CNT)

// Struct for states
typedef enum {
    ON,
    INITIALIZATION,
    READING,
    HRBO,
    TRANSMIT
} device_state_t;

// Vibration functions
void vibration_init();
void check_vibrate();
void start_vibration();

// Functions to access peripherals
void collect_muscle_data(uint32_t *average, bool timed);
void sense_HRBO(float *average_bpm, float *blood_oxygen);
void reset_globals();

// Interrupt and timer functions
void RTC_init(void);
ISR(PORTA_PORT_vect);
ISR(RTC_CNT_vect);

// Main function
int main();


#endif	/* NEWAVIR_MAIN_H */


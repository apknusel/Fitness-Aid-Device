#include "button_led.h"

/**
 * Initialize the buttons
 */
void button_init() {
    PORTA.DIR &= ~PIN6_bm;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
    PORTA.DIR &= ~PIN4_bm;
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
    RED_BUTTON_INTERRUPT_CLEAR;
    YELLOW_BUTTON_INTERRUPT_CLEAR;
}

/**
 * Function to set the LED color
 * @param red 1 for on, 0 for off
 * @param green 1 for on, 0 for off
 * @param blue 1 for on, 0 for off
 */
void set_LED_color(uint8_t red, uint8_t green, uint8_t blue) {
    // Control the red channel (Port D7)
    if (red) {
        PORTD.OUTSET = PIN7_bm; // Turn on red
    } else {
        PORTD.OUTCLR = PIN7_bm; // Turn off red
    }

    // Control the green channel (Port D5)
    if (green) {
        PORTD.OUTSET = PIN5_bm; // Turn on green
    } else {
        PORTD.OUTCLR = PIN5_bm; // Turn off green
    }

    // Control the blue channel (Port A7)
    if (blue) {
        PORTA.OUTSET = PIN7_bm; // Turn on blue
    } else {
        PORTA.OUTCLR = PIN7_bm; // Turn off blue
    }
}

/**
 * Initialize the LED
 */
void LED_init() {
    // Set Port D7, D5, and A7 as output
    PORTD.DIRSET = PIN7_bm | PIN5_bm;
    PORTA.DIRSET = PIN7_bm;
}

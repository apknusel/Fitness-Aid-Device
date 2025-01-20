#include "muscle.h"

/**
 * Initialize ADC communication on pin D1
 */
void ADC_init() {
    // DISABLE DIGITAL INPUT BUFFER
    PORTD.PIN1CTRL &= ~PORT_ISC_gm;
    PORTD.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    
    // DISABLE PULLUP RESISTOR
    PORTD.PIN1CTRL &= ~PORT_PULLUPEN_bm;
    
    // SELECT ADC CHANNEL AIN1 = PD1
    ADC0.MUXPOS  = ADC_MUXPOS_AIN1_gc;
    
    // SET INTERNAL REFERENCE AND DIVIDE BY 4
    ADC0.CTRLC = ADC_PRESC_DIV4_gc
               | ADC_REFSEL_VDDREF_gc;
   
    // ENABLE ADC IN FREERUN AND 10-BIT MODE
    ADC0.CTRLA = ADC_RESSEL_10BIT_gc
               | ADC_ENABLE_bm;
}

/**
 * Read from the ADC (value is from 0 to 1023)
 * @return ADC reading
 */
uint16_t ADC_read() {
    // Start conversion
    ADC0.COMMAND = ADC_STCONV_bm;

    // Wait for conversion to complete
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) {
        // Wait for result ready flag
    }

    // Clear result ready flag
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    // Return ADC result
    return ADC0.RES;
}
#include "max30102_math.h"
/*
 * The code for the avg_DC_estimator function and low_pass_FIR_filter is adapted
 * from https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/tree/master.
 * I tested my own values to get the readings more accurate for my specific
 * MAX30102 sensor and the frequency of readings the board was doing.
 * I also rewrote some parts for more efficiency and readability.
 * I wrote my own code for calculating spo2 (blood oxygen)
 */

// Global variables for heart rate detection
int16_t IR_AC_Max = 30;
int16_t IR_AC_Min = -30;

int16_t IR_AC_Signal_Current = 0;
int16_t IR_AC_Signal_Previous;
int16_t IR_AC_Signal_min = 0;
int16_t IR_AC_Signal_max = 0;
int16_t IR_Average_Estimated;

int16_t positiveEdge = 0;
int16_t negativeEdge = 0;
int32_t ir_avg_reg = 0;

int16_t cbuf[32];
uint8_t offset = 0;

float spo2_running_average = 0.0;
uint32_t spo2_sample_count = 0;

// FIRCoeffs taken from https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/tree/master
static const uint16_t FIRCoeffs[12] = {172, 321, 579, 927, 1360, 1858, 2390, 2916, 3391, 3768, 4012, 4096};

/**
 * Low Pass FIR Filter
 * @param din input value
 * @return resulting value
 */
int16_t low_pass_FIR_filter(int16_t din) {
    // Update the circular buffer with the new input sample
    cbuf[offset] = din;

    // Compute the center tap multiplication
    int32_t z = FIRCoeffs[11] * (cbuf[(offset - 11) & 0x1F]);

    // Compute the weighted sum of symmetric taps
    for (uint8_t i = 0; i < 11; i++) {
        z += FIRCoeffs[i] * (cbuf[(offset - i) & 0x1F] + cbuf[(offset - 22 + i) & 0x1F]);
    }

    // Increment and wrap the offset for the circular buffer
    offset++;
    offset %= 32;

    // Scale back to 16 bits and return the result
    return (z >> 15);
}

/**
 * Average DC Estimator
 * @param dc_component DC component
 * @param input_value value to be processed
 * @return updated DC component
 */
int16_t avg_DC_estimator(int32_t *dc_component, uint32_t input_value) {
    // Scale the input value to a fixed-point representation (shift left by 15 bits)
    int32_t scaled_input = (int32_t)input_value << 15;

    // Update the DC component using a smoothing factor
    *dc_component += (scaled_input - *dc_component) >> 4;

    // Return the DC component scaled back to the original range
    return (int16_t)(*dc_component >> 15);
}

/**
 * Check if a finger/beat is present
 * @param ir_val the ir value from a sample
 * @return bool bool representing if a beat is present
 */
bool check_for_beat(int32_t ir_val) {
    bool beatDetected = false;

    IR_AC_Signal_Previous = IR_AC_Signal_Current;

    // Process next data sample
    IR_Average_Estimated = avg_DC_estimator(&ir_avg_reg, ir_val);
    IR_AC_Signal_Current = low_pass_FIR_filter(ir_val - IR_Average_Estimated);

    // Detect positive zero crossing (rising edge)
    if ((IR_AC_Signal_Previous < 0) && (IR_AC_Signal_Current >= 0)) {
        // Adjust AC max and min
        IR_AC_Max = IR_AC_Signal_max;
        IR_AC_Min = IR_AC_Signal_min;

        positiveEdge = 1;
        negativeEdge = 0;
        IR_AC_Signal_max = 0;

        if ((IR_AC_Max - IR_AC_Min) > 40 && (IR_AC_Max - IR_AC_Min) < 1300) {
            beatDetected = true;
        }
    }

    // Detect negative zero crossing (falling edge)
    if ((IR_AC_Signal_Previous > 0) && (IR_AC_Signal_Current <= 0)) {
        positiveEdge = 0;
        negativeEdge = 1;
        IR_AC_Signal_min = 0;
    }

    // Find max value in positive cycle
    if (positiveEdge && (IR_AC_Signal_Current > IR_AC_Signal_Previous)) {
        IR_AC_Signal_max = IR_AC_Signal_Current;
    }

    // Find min value in negative cycle
    if (negativeEdge && (IR_AC_Signal_Current < IR_AC_Signal_Previous)) {
        IR_AC_Signal_min = IR_AC_Signal_Current;
    }

    return beatDetected;
}

/**
 * Handles calculating the blood oxygen
 * @param ir_val ir value from sample
 * @param red_val red value from sample
 */
void calculate_and_update_spo2(int32_t ir_val, int32_t red_val, float *spo2_average_out) {
    static int32_t ir_avg_reg = 0, red_avg_reg = 0;

    // Estimate DC and AC components
    int16_t DC_IR = avg_DC_estimator(&ir_avg_reg, ir_val);
    int16_t AC_IR = low_pass_FIR_filter(ir_val - DC_IR);

    int16_t DC_Red = avg_DC_estimator(&red_avg_reg, red_val);
    int16_t AC_Red = low_pass_FIR_filter(red_val - DC_Red);

    // Avoid division by zero
    if (DC_Red == 0 || DC_IR == 0 || AC_IR == 0) {
        return;
    }

    // Calculate the R value
    float R = ((float)AC_Red / DC_Red) / ((float)AC_IR / DC_IR);

    // Compute blood oxygen with calibrated coefficients
    float spo2 = 110.0 - 25.0 * R;

    // Clamp values to realistic range
    if (spo2 > 100.0) {
        spo2 = 100.0;
    }
    if (spo2 < 70.0) {
        spo2 = 70.0;
    }

    // Update running average
    spo2_sample_count++;
    spo2_running_average += (spo2 - spo2_running_average) / spo2_sample_count;

    // Update the output parameter
    *spo2_average_out = spo2_running_average;
}

/**
 * Reset globals to allow for a new reading
 */
void reset_math_globals() {
    // Reset heart rate detection variables
    IR_AC_Max = 20;
    IR_AC_Min = -20;

    IR_AC_Signal_Current = 0;
    IR_AC_Signal_Previous = 0;
    IR_AC_Signal_min = 0;
    IR_AC_Signal_max = 0;
    IR_Average_Estimated = 0;

    positiveEdge = 0;
    negativeEdge = 0;
    ir_avg_reg = 0;

    // Reset circular buffer and offset
    for (int i = 0; i < 32; i++) {
        cbuf[i] = 0;
    }
    offset = 0;
}
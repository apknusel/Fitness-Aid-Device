#include <stdint.h>
#include <stdbool.h>

#ifndef MAX30102_MATH_H
#define	MAX30102_MATH_H

int16_t low_pass_FIR_filter(int16_t din);
int16_t avg_DC_estimator(int32_t *dc_component, uint32_t input_value);
bool check_for_beat(int32_t ir_val);
void reset_math_globals();
void calculate_and_update_spo2(int32_t ir_val, int32_t red_val, float *spo2_average_out);

#endif	/* MAX30102_MATH_H */


#ifndef F_CPU
#define F_CPU 3333333
#endif

#include "newavr-main.h"

// Variable that holds the state of the device
volatile device_state_t device_state;

// Variables to implement delay between states and vibration
volatile uint32_t button_press_time = 0; // Time of the button press
volatile bool state_change_pending = false; // Flag for pending state change
volatile device_state_t next_state; // Holds the next state to switch to
volatile uint32_t vibrate_start_time = 0; // Time for when the vibration starts
volatile bool is_vibrating = false; //  Flag for vibration
volatile bool automatic_transition = 0; // Flag for automatic transitions

// Variables for muscle sensor data
volatile uint32_t initialization_start_time = 0; // Start time of the INITIALIZATION state
volatile uint32_t muscle_sum = 0; // Sum of muscle sensor readings
volatile uint32_t muscle_samples = 0; // Number of muscle sensor samples
volatile uint32_t baseline_muscle_average = 0; // Baseline average muscle reading
volatile uint32_t muscle_average = 0; // Average muscle reading

// Variables for calculating heart rate
volatile float average_bpm = 0; // Average beats per minute
volatile float blood_oxygen = 0; // Average blood oxygen level
volatile long lastBeat = 0; // Time since the last beat
volatile float beatsPerMinute = 0; // Current beats per minute
volatile uint8_t rates[RATE_SIZE] = {0}; // Buffer for averaging
volatile uint8_t rateSpot = 0; // Index in rates buffer
volatile uint32_t ir_start_time = 0; // Time when red value exceeded threshold
volatile bool ir_below_threshold = false; // Tracks if red value is above threshold

// Buffer for storing the data to send over BLE
volatile uint16_t data_to_send[4];

// Amount of time in the READING state
volatile uint32_t reading_time = 0;

// Variable to hold milliseconds since startup
volatile uint32_t milliseconds = 0;

/**
* Initialize the vibrating peripheral
*/
void vibration_init() {
   PORTA.DIR |= PIN5_bm;
}

/**
* Starts the vibrating peripheral
*/
void start_vibration() {
   PORTA.OUTSET = PIN5_bm; // Set A5 high
   vibrate_start_time = millis();  // Record the start time
   is_vibrating = true; // Mark vibration as active
}

/**
* Checks if the device is vibrating and cancels it after 500 ms
*/
void check_vibrate() {
   if (is_vibrating) {
       // Check if 500 ms have elapsed
       if (millis() - vibrate_start_time >= 500) {
           PORTA.OUTCLR = PIN5_bm; // Set A5 low (turn off output)
           is_vibrating = false; // Reset the state
       }
   }
}

/**
* Initializes the RTC timer
*/
void RTC_init(void) {
   // Select RTC clock source
   RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;

   // Configure RTC: Enable, run in standby, prescaler = 32 so 1024 Hz
   RTC.CTRLA = RTC_PRESCALER_DIV32_gc | RTC_RUNSTDBY_bm | RTC_RTCEN_bm;

   // Enable overflow interrupt
   RTC.INTCTRL = RTC_OVF_bm;

   // Start from zero
   RTC.CNT = 0;
   
   // Interrupt every second
   RTC.PER = 1024;
}

/**
* Collects the muscle data from the muscle sensor peripheral
* @param average holds the average reading for the muscle sensor
* @param timed if the function should run for 3 seconds or until state change
*/
void collect_muscle_data(uint32_t *average, bool timed) {
   // Start initialization process
   if (initialization_start_time == 0) {
       initialization_start_time = millis(); // Record the start time
       muscle_sum = 0;
       muscle_samples = 0;
   }


   // If timed is true and 3 seconds have elapsed, finalize data collection,
   // otherwise a button press will end this looping
   if (timed && millis() - initialization_start_time >= 3000) {
        if (muscle_samples > 0) {
            *average = muscle_sum / muscle_samples; // Update the variable passed as an argument
            set_LED_color(0, 1, 0);
            start_vibration();
        } else {
            *average = 0; // Set to 0 if no samples were collected
        }
        // Reset for future use if stopping after 3 seconds
        initialization_start_time = 0;
        next_state = ON;
        automatic_transition = true;
   } else {
       muscle_sum += ADC_read();
       muscle_samples++;
   }
}

/**
* Updates the variables passed in with the correct data for the HR and BO
* @param average_bpm variable to hold the average beats per minute
* @param blood_oxygen variable to hold the average blood oxygen
*/
void sense_HRBO(float *average_bpm, float *blood_oxygen) {
   MAX30102_sample_t sample;
   MAX30102_get_sample(&sample);

   // Heart rate calculation
   if (check_for_beat(sample.ir)) {
       uint32_t delta = millis() - lastBeat;
       lastBeat = millis();

       beatsPerMinute = 60 / (delta / 1000.0);

       if (beatsPerMinute < 255 && beatsPerMinute > 20) {
           rates[rateSpot++] = (uint8_t)beatsPerMinute;
           rateSpot %= RATE_SIZE;

           // Calculate the average BPM
           *average_bpm = 0;
           for (uint8_t i = 0; i < RATE_SIZE; i++) {
               *average_bpm += rates[i];
           }
           *average_bpm /= RATE_SIZE;
           // Blood oxygen calculation
           calculate_and_update_spo2(&blood_oxygen, sample.ir, sample.red);
       }
   }

   // Transition to TRANSMIT state if red value < 50000 for 3 seconds
   if (sample.ir < 50000) {
        if (!ir_below_threshold) {
            ir_start_time = millis(); // Start timing
            ir_below_threshold = true;
        } else if (millis() - ir_start_time >= 3000) {
            set_LED_color(1, 0, 1); // Set LED to Purple
            ir_below_threshold = false; // Reset tracking
            ir_start_time = 0;
            next_state = TRANSMIT; // Transition to TRANSMIT state
            automatic_transition = true;
        }
   } else {
       // Reset if red value drops below threshold
       ir_below_threshold = false;
       ir_start_time = 0;
   }
}

/**
 * Resets all the variables that need to be rest in between readings
 */
void reset_globals() {
    muscle_average = 0;
    average_bpm = 0;
    blood_oxygen = 0;
    reading_time = 0;
    lastBeat = 0;
    beatsPerMinute = 0;
    rateSpot = 0;
    ir_start_time = 0;
    ir_below_threshold = false;
    initialization_start_time = 0;
    for (uint8_t i = 0; i < RATE_SIZE; i++) {
        rates[i] = 0;
    }
}
/**
* Port interrupt service routine for button press/release
*/
ISR(PORTA_PORT_vect) {
   if (RED_BUTTON_INTERRUPT) {
       if (device_state == ON && !state_change_pending) {
           button_press_time = millis(); // Record button press time
           next_state = INITIALIZATION; // Set the next state
           state_change_pending = true; // Indicate a pending state change
       }
       RED_BUTTON_INTERRUPT_CLEAR;
   }

   if (YELLOW_BUTTON_INTERRUPT) {
       if ((device_state == ON || device_state == READING || device_state == HRBO) && !state_change_pending) {
           state_change_pending = true; // Indicate a pending state change
           button_press_time = millis(); // Record button press time
           switch (device_state) {
               case ON:
                   next_state = READING;
                   reading_time = millis();
                   break;
               case READING:
                   next_state = HRBO;
                   reading_time = millis() - reading_time;
                   break;
               case HRBO:
                   next_state = TRANSMIT;
                   break;
           }
       }
       YELLOW_BUTTON_INTERRUPT_CLEAR;
   }
}

/**
* RTC interrupt to keep track of system time
*/
ISR(RTC_CNT_vect) {
   // Overflow is trigger every 1000 ms
   milliseconds += 1000;
   RTC.INTFLAGS = RTC_OVF_bm; // Clear overflow flag
}

/**
* Main function to initialize and run the code
*/
int main() {
   // Initializations
   USART_init();
   RTC_init();
   ADC_init();
   MAX30102_init();
   MAX30102_setup();
   button_init();
   vibration_init();
   LED_init();
   BLE_init("FitDev");
   
   // Enable interrupts
   sei();
   
   // Initialize to ON state;
   device_state = ON;
   set_LED_color(0, 1, 0); // Green

   while (1) {
       // Check if a state change is pending and if the delay has elapsed
       if ((state_change_pending && (millis() - button_press_time >= 3000)) || automatic_transition) {
            device_state = next_state; // Switch to the next state
            state_change_pending = false; // Reset the flag
            automatic_transition = false;
            start_vibration();

            // Change the LED based on the new state
            switch (device_state) {
                case ON:
                    set_LED_color(0, 1, 0); // Green
                    break;
                case INITIALIZATION:
                    set_LED_color(1, 1, 0); // Yellow
                    break;
                case READING:
                    set_LED_color(0, 0, 1); // Blue
                    break;
                case HRBO:
                    set_LED_color(1, 0, 0); // Red
                    break;
                case TRANSMIT:
                    set_LED_color(1, 0, 1); // Purple
                    break;
            }
        }
       // Check if the device should vibrate
       check_vibrate();
       
       // General brains of the code to determine what to do in each state
       switch (device_state) {
           case ON:
               break;
           case INITIALIZATION:
               collect_muscle_data(&baseline_muscle_average, true);
               break;
           case READING:
               collect_muscle_data(&muscle_average, false);
               break;
           case HRBO:
               sense_HRBO(&average_bpm, &blood_oxygen);
               break;
           case TRANSMIT:
               data_to_send[0] = muscle_average/baseline_muscle_average*100;
               data_to_send[1] = average_bpm;
               data_to_send[2] = blood_oxygen;
               data_to_send[3] = reading_time / 1000;
               BLE_send_data(data_to_send, 4);
               set_LED_color(0, 1, 0); // Set color to green
               next_state = ON;
               automatic_transition = true;
               reset_math_globals();
               reset_globals();
               break;
       }
   }
}

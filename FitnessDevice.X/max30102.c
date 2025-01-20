#include "max30102.h"

/**
 * Function for applying bit mask to a register
 * @param reg register to update
 * @param mask bitmask to apply
 * @param value value that is being masked
 */
void MAX30102_bitMask(uint8_t reg, uint8_t mask, uint8_t value) {
    uint8_t currentValue = MAX30102_readRegister8(reg);
    currentValue &= mask;
    currentValue |= value;
    MAX30102_writeRegister8(reg, currentValue);
}

/**
 * Soft reset the MAX30102
 */
void MAX30102_softReset() {
    // Trigger soft reset
    MAX30102_bitMask(MAX30105_MODECONFIG, MAX30105_RESET_MASK, MAX30105_RESET);

    // Wait until the reset bit is cleared
    while ((MAX30102_readRegister8(MAX30105_MODECONFIG) & MAX30105_RESET) != 0) {;}
}

/**
 * Sets up the MAX30102 for reading from
 */
void MAX30102_setup() {
    // Perform a soft reset
    MAX30102_softReset();

    // Configure FIFO settings
    MAX30102_bitMask(MAX30105_FIFOCONFIG, MAX30105_SAMPLEAVG_MASK, MAX30105_SAMPLEAVG_4); // Set sample average to 4 (default)
    MAX30102_bitMask(MAX30105_FIFOCONFIG, MAX30105_ROLLOVER_MASK, MAX30105_ROLLOVER_ENABLE); // Enable FIFO rollover

    // Configure mode
    MAX30102_bitMask(MAX30105_MODECONFIG, MAX30105_MODE_MASK, MAX30105_MODE_MULTILED); // Set to Multi-LED mode (default)
    
    // ADC 62.5pA per LSB
    MAX30102_bitMask(MAX30105_PARTICLECONFIG, MAX30105_ADCRANGE_MASK, MAX30105_ADCRANGE_4096);

    // Set sample rate
    MAX30102_bitMask(MAX30105_PARTICLECONFIG, MAX30105_SAMPLERATE_MASK, MAX30105_SAMPLERATE_100);
    
    // Set pulse width to 69us (default)
    MAX30102_bitMask(MAX30105_PARTICLECONFIG, MAX30105_PULSEWIDTH_MASK, MAX30105_PULSEWIDTH_411);

    // Configure LED pulse amplitudes
    
    
    MAX30102_writeRegister8(MAX30105_LED1_PULSEAMP, DEFAULT_POWER_LEVEL); // Red LED
    MAX30102_writeRegister8(MAX30105_LED2_PULSEAMP, DEFAULT_POWER_LEVEL); // IR LED
    MAX30102_writeRegister8(MAX30105_LED3_PULSEAMP, DEFAULT_POWER_LEVEL); // Green LED
    MAX30102_writeRegister8(MAX30105_LED_PROX_AMP, DEFAULT_POWER_LEVEL); // Proximity LED

    MAX30102_bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT1_MASK, SLOT_RED_LED);
    MAX30102_bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT2_MASK, SLOT_IR_LED << 4);
    MAX30102_bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT3_MASK, SLOT_GREEN_LED);

    // Clear FIFO
    MAX30102_writeRegister8(MAX30105_FIFOWRITEPTR, 0x00); // FIFO Write Pointer
    MAX30102_writeRegister8(MAX30105_FIFOOVERFLOW, 0x00); // FIFO Read Pointer
    MAX30102_writeRegister8(MAX30105_FIFOREADPTR, 0x00); // FIFO Overflow Counter
    
    // For HR
    MAX30102_writeRegister8(MAX30105_LED1_PULSEAMP, 0x0A); // Red LED
    MAX30102_writeRegister8(MAX30105_LED3_PULSEAMP, 0); // Green LED
}

/**
 * Initializes the ports for communicating with MAX30102
 */
void MAX30102_init() {
    // Set pin output directions
    PORTA.DIRSET = PIN2_bm | PIN3_bm;
    
    // Enable pullup resistors
    PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
    PORTA.PIN3CTRL |= PORT_PULLUPEN_bm;
    // Baud rate for 100kHz
    TWI0.MBAUD = 11;
    TWI0.MCTRLA = TWI_ENABLE_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;     
}

/**
 * Request data from the circular buffer on the MAX30102
 * @param toGet
 * @param buffer
 */
void MAX30102_buffer_data(uint8_t toGet, uint8_t *buffer) {
    // Step 1: Send START condition and write address to set register pointer
    TWI0.MADDR = (MAX30102_I2C_ADDR << 1) | 0; // Address with WRITE bit

    // Wait for acknowledgment
    while (!(TWI0.MSTATUS & TWI_WIF_bm));

    // Write the register address
    TWI0.MDATA = MAX30105_FIFODATA; // Specify the FIFO data register
    while (!(TWI0.MSTATUS & TWI_WIF_bm)); // Wait for acknowledgment

    // Step 2: Send repeated START and switch to READ mode
    TWI0.MCTRLB = TWI_MCMD_REPSTART_gc; // Send repeated START
    TWI0.MADDR = (MAX30102_I2C_ADDR << 1) | 1; // Address with READ bit

    // Wait for acknowledgment
    while (!(TWI0.MSTATUS & TWI_RIF_bm));

    // Step 3: Read data into the buffer
    for (uint8_t i = 0; i < toGet; i++) {
        // Wait for data reception
        while (!(TWI0.MSTATUS & TWI_RIF_bm));
        buffer[i] = TWI0.MDATA; // Read data into buffer

        // Acknowledge or NACK the data
        if (i == toGet - 1) {
            TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc; // Send NACK and STOP on last byte
        } else {
            TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc; // Send ACK to continue receiving
        }
    }
}

/**
 * Read from a register on the MAX30102
 * @param reg The register to read from
 * @return 
 */
uint8_t MAX30102_readRegister8(uint8_t reg) {
    // Ensure bus is idle
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_IDLE_gc) {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc; // Send STOP to reset bus
        TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc; // Force bus to idle
    }

    // Send START condition and slave address with write bit
    TWI0.MADDR = (MAX30102_I2C_ADDR << 1); // Write operation
    while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm)));

    if (TWI0.MSTATUS & TWI_RXACK_bm) {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return 0xFF; // NACK received
    }

    // Send register address
    TWI0.MDATA = reg;
    while (!(TWI0.MSTATUS & TWI_WIF_bm));
    if (TWI0.MSTATUS & TWI_RXACK_bm) {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return 0xFF; // NACK received
    }

    // Send repeated START condition and slave address with read bit
    TWI0.MADDR = (MAX30102_I2C_ADDR << 1) | 1; // Read operation
    while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm)));
    if (TWI0.MSTATUS & TWI_RXACK_bm) {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return 0xFF; // NACK received
    }

    // Read data
    while (!(TWI0.MSTATUS & TWI_RIF_bm));
    uint8_t data = TWI0.MDATA;

    // Send NACK and STOP condition
    TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;

    return data;
}

/**
 * Write to a register on the MAX30102
 * @param reg
 * @param value
 */
void MAX30102_writeRegister8(uint8_t reg, uint8_t value) {
    // Ensure bus is idle
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_IDLE_gc) {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc; // Send STOP to reset bus
        TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc; // Force bus to idle
    }

    // Send START condition and slave address with write bit
    TWI0.MADDR = (MAX30102_I2C_ADDR << 1); // Write operation
    while (!(TWI0.MSTATUS & TWI_WIF_bm));

    if (TWI0.MSTATUS & TWI_RXACK_bm) {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return; // NACK received
    }

    // Send register address
    TWI0.MDATA = reg;
    while (!(TWI0.MSTATUS & TWI_WIF_bm));

    if (TWI0.MSTATUS & TWI_RXACK_bm) {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return; // NACK received
    }

    // Send data
    TWI0.MDATA = value;
    while (!(TWI0.MSTATUS & TWI_WIF_bm));

    if (TWI0.MSTATUS & TWI_RXACK_bm) {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return; // NACK received
    }

    // Send STOP condition
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

/**
 * Get a sample of ir, red, and green values from the MAX30102
 * @param sample
 */
void MAX30102_get_sample(MAX30102_sample_t *sample) {
    volatile uint8_t buffer[9];
    volatile uint32_t tempLong;
    volatile uint8_t temp[4];

    // Read 9 bytes into buffer
    MAX30102_buffer_data(9, buffer);

    // Combine the 3 bytes into a single 32-bit value for red LED
    temp[3] = 0;
    temp[2] = buffer[0];
    temp[1] = buffer[1];
    temp[0] = buffer[2];
    memcpy(&tempLong, temp, sizeof(tempLong));
    tempLong &= 0x3FFFF; // Mask to 18 bits
    sample->red = tempLong;
    
    // Combine the 3 bytes into a single 32-bit value for IR LED
    temp[3] = 0;
    temp[2] = buffer[3];
    temp[1] = buffer[4];
    temp[0] = buffer[5];
    memcpy(&tempLong, temp, sizeof(tempLong));
    tempLong &= 0x3FFFF; // Mask to 18 bits
    sample->ir = tempLong;
    
    // Combine the 3 bytes into a single 32-bit value for green LED
    temp[3] = 0;
    temp[2] = buffer[6];
    temp[1] = buffer[7];
    temp[0] = buffer[8];
    memcpy(&tempLong, temp, sizeof(tempLong));
    tempLong &= 0x3FFFF; // Mask to 18 bits
    sample->green = tempLong;
}

/**
 * Reset the circular buffer and clear it on the MAX30102
 */
void MAX30102_clearFIFO() {
    MAX30102_writeRegister8(0x04, 0x00); // FIFO Write Pointer
    MAX30102_writeRegister8(0x05, 0x00); // FIFO Read Pointer
    MAX30102_writeRegister8(0x06, 0x00); // FIFO Overflow Counter
}
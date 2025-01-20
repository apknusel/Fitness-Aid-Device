#include "bluetooth.h"

/**
 * Initialize USART
 */
void USART_init() {
    USART0.BAUD = USART_BAUD_VALUE(9600);
    
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
    
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc
            | USART_CHSIZE_8BIT_gc;

    PORTA.DIR |= PIN0_bm;
    PORTA.DIR &= ~PIN1_bm;
}

/**
 * Writes a single char to USART
 * @param c char to write
 */
void usartWriteChar(char c) {
    // TODO fill this in
    while (!(USART0.STATUS & USART_DREIF_bm)) { ; }        
    USART0.TXDATAL = c;
}

/**
 * Writes a command to USART
 * @param cmd string command to write
 */
void usartWriteCommand(const char *cmd) {
    for (uint8_t i = 0; cmd[i] != '\0'; i++) {
        usartWriteChar(cmd[i]);
    }
}

/**
 * Reads a single char from USART
 * @return char from USART
 */
char usartReadChar() {
    while (!(USART0.STATUS & USART_RXCIF_bm));
    return USART0.RXDATAL;
}

/**
 * 
 * @param dest where to read the bytes too
 * @param end_str string signifying the end of transmission
 */
void usartReadUntil(char *dest, const char *end_str) {
    // Zero out dest memory so we always have null terminator at end
    memset(dest, 0, BUF_SIZE);
    uint8_t end_len = strlen(end_str);
    uint8_t bytes_read = 0;
    while (bytes_read < end_len || strcmp(dest + bytes_read - end_len, end_str) != 0) {
        dest[bytes_read] = usartReadChar();
        bytes_read++;
    }
}

/**
 * Initialize BLE communication
 * @param name name of BLE Device to broadcast
 */
void BLE_init(const char *name) {
    // Put BLE Radio in "Application Mode" by driving F3 high
    PORTF.DIRSET = PIN3_bm;
    PORTF.OUTSET = PIN3_bm;

    // Reset BLE Module - pull PD3 low, then back high after a delay
    PORTD.DIRSET = PIN3_bm | PIN2_bm;
    PORTD.OUTCLR = PIN3_bm;
    _delay_ms(10); // Leave reset signal pulled low
    PORTD.OUTSET = PIN3_bm;

    // The AVR-BLE hardware guide is wrong. Labels this as D3
    // Tell BLE module to expect data - set D2 low
    PORTD.OUTCLR = PIN2_bm;
    _delay_ms(200); // Give time for RN4870 to boot up

    char buf[BUF_SIZE];
    // Put RN4870 in Command Mode
    usartWriteCommand("$$$");
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    // Change BLE device name to specified value
    strcpy(buf, "S-,");
    strcat(buf, name);
    strcat(buf, "\r\n");
    usartWriteCommand(buf);
    usartReadUntil(buf, BLE_RADIO_PROMPT);

    // Send command to remove all previously declared BLE services
    usartWriteCommand("PZ\r\n");
    usartReadUntil(buf, "AOK\r\n");
    // Fitness Machine Service
    usartWriteCommand("PS,1826\r\n");
    usartReadUntil(buf, "AOK\r\n");
    // Physical Activity Level Characteristic
    usartWriteCommand("PC,2AD2,1C,20\r\n");
    // USED TO GET THE ADDRESS OF THE CHARACTERISTIC
    usartReadUntil(buf, "AOK\r\n");
    usartWriteCommand("LS,2AD2\r\n");
    usartReadUntil(buf, BLE_RADIO_PROMPT);
    // Set the characteristic's initial value to hex "00".
    usartWriteCommand("SHW,0072,00\r\n");
    usartReadUntil(buf, "AOK\r\n");
}

/**
 * Sends data over BLE
 * @param data The array of data to be sent
 * @param length The length of the data being sent
 */
void BLE_send_data(uint16_t *data, size_t length) {
    char buf[BUF_SIZE];
    char command[BUF_SIZE];
    for (int i = 0; i < length; i++) {
        snprintf(command, sizeof(command), "SHW,0072,%04X\r\n", data[i]);
        usartWriteCommand(command);
        usartReadUntil(buf, BLE_RADIO_PROMPT);
    }
}
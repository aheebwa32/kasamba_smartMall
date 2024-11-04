#define ROW1 PD4
#define ROW2 PD5
#define ROW3 PD6
#define ROW4 PD7

#define COL1 PE2
#define COL2 PE3
#define COL3 PE4

void keypad_init(void) {
    // Set rows as outputs
    DDRD |= (1 << ROW1) | (1 << ROW2) | (1 << ROW3) | (1 << ROW4);
    // Set columns as inputs
    DDRE &= ~((1 << COL1) | (1 << COL2) | (1 << COL3));
    PORTE |= (1 << COL1) | (1 << COL2) | (1 << COL3);
}

char keypad_scan(void) {
    const char keys[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };

    for (uint8_t row = 0; row < 4; row++) {
        // Set the current row to low
        PORTD &= ~((1 << ROW1) << row);
        _delay_us(5); // Small delay to allow the signal to settle

        // Check each column
        for (uint8_t col = 0; col < 3; col++) {
            if (!(PINE & (1 << (COL1 + col)))) {
                // Restore the row to high
                PORTD |= (1 << ROW1) << row;
                return keys[row][col];
            }
        }

        // Restore the row to high
        PORTD |= (1 << ROW1) << row;
    }

    return NULL; // No key pressed
}
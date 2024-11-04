#define LCD_RS PH5
#define LCD_RW PH6
#define LCD_EN PH7
#define LCD_CMDS PORTH
#define LCD_DATA PORTK

void stall(void) {
    LCD_CMDS |= (1 << LCD_EN);
    _delay_ms(1);
    LCD_CMDS &= ~(1 << LCD_EN);
    _delay_ms(1);
}

void lcd_send_command(unsigned char command) {
    LCD_DATA = command;
    LCD_CMDS &= ~(1 << LCD_RS); // RS = 0 for command
    LCD_CMDS &= ~(1 << LCD_RW); // RW = 0 for write
    stall();
}

void lcd_send_data(unsigned char data) {
    LCD_DATA = data;
    LCD_CMDS |= (1 << LCD_RS);  // RS = 1 for data
    LCD_CMDS &= ~(1 << LCD_RW); // RW = 0 for write
    stall();
}

void lcd_clear() {
    lcd_send_command(0x01); // Clear display
}

void lcd_print(char* message) {
    if (sizeof(message) == 0) return;
    while (*message) {
        lcd_send_data(*message++);
    }
}

void lcd_init() {
    DDRK = 0xFF; // Set PORTK as output for data
    // DDRH = 0xE0; // Set PORTH as output for control signals

    _delay_ms(20); // Wait for LCD to power up

    lcd_send_command(0x38); // Function set: 8-bit, 2 line, 5x7 dots
    lcd_send_command(0x0C); // Display on, cursor off
    lcd_clear();
    _delay_ms(2);
    LCD_DATA = 0;
    lcd_send_command(0x06); // Entry mode: increment cursor

    char* welcome = "KASAMBA S.M.";
    lcd_print(welcome);
}

void lcd_display_number(int number) {
    char buffer[16];
    itoa(number, buffer, 10); // Convert number to string
    for (char *ptr = buffer; *ptr != '\0'; ptr++) {
        lcd_send_data(*ptr);
    }
}

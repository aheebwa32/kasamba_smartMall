#define LCD_RS PH5
#define LCD_RW PH6
#define LCD_EN PH7
#define LCD2_RS PJ5
#define LCD2_RW PJ6
#define LCD2_EN PJ7
#define LCD_CMDS PORTH
#define LCD2_CMDS PORTJ
#define LCD_DATA PORTK

void stall(void) {
    LCD_CMDS |= (1 << LCD_EN);
    _delay_ms(1);
    LCD_CMDS &= ~(1 << LCD_EN);
    _delay_ms(1);
}

void stall2(void) {
    LCD2_CMDS |= (1 << LCD2_EN);
    _delay_ms(1);
    LCD2_CMDS &= ~(1 << LCD2_EN);
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
        if (*message != '\0') lcd_send_data(*message++);
    }
}

// washroom lcd
void lcd_send_command2(unsigned char command) {
    PORTL = command;
    LCD2_CMDS &= ~(1 << LCD2_RS); // RS = 0 for command
    LCD2_CMDS &= ~(1 << LCD2_RW); // RW = 0 for write
    stall2();
}

void lcd_send_data2(unsigned char data) {
    PORTL = data;
    LCD2_CMDS |= (1 << LCD2_RS);  // RS = 1 for data
    LCD2_CMDS &= ~(1 << LCD2_RW); // RW = 0 for write
    stall2();
}

void lcd_clear2() {
    lcd_send_command2(0x01); // Clear display
}

void lcd_print2(char* message) {
    if (sizeof(message) == 0) return;
    while (*message) {
        if (*message != '\0') lcd_send_data2(*message++);
    }
}

void lcd_init() {
    DDRK = 0xFF; // Set PORTK as output for data
    DDRL = 0xFF; // Set PORTL as output for data for washrooms
    // DDRH = 0xE0; // Set PORTH as output for control signals
    DDRJ = 0XE0; // Set PORTJ as output for control signals for washrooms

    _delay_ms(20); // Wait for LCD to power up

    lcd_send_command(0x38); // Function set: 8-bit, 2 line, 5x7 dots
    lcd_send_command2(0x38); // Function set: 8-bit, 2 line, 5x7 dots
    lcd_send_command(0x0C); // Display on, cursor off
    lcd_send_command2(0x0C); // Display on, cursor off
    lcd_clear();
    lcd_clear2();
    _delay_ms(2);
    LCD_DATA = 0;
    PORTL = 0;
    lcd_send_command(0x06); // Entry mode: increment cursor
    lcd_send_command2(0x06); // Entry mode: increment cursor

    char* welcome = "KASAMBA S.M.";
    lcd_print(welcome);
    char* welcome2 = "CODE";
    lcd_print2(welcome2);
}

void lcd_display_number(int number) {
    char buffer[16];
    itoa(number, buffer, 10); // Convert number to string
    for (char *ptr = buffer; *ptr != '\0'; ptr++) {
        lcd_send_data(*ptr);
    }
}

void lcd_display_number2(int number) {
    char buffer[16];
    itoa(number, buffer, 10); // Convert number to string
    for (char *ptr = buffer; *ptr != '\0'; ptr++) {
        lcd_send_data2(*ptr);
    }
}

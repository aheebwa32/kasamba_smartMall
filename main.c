// main.c
// Authors: Agani, Conrad, Steve, Samuel
#include "smart_mall.h"
#include "uart.h"
#include "lcd.h"

// Global variable initialization
Tenant tenants[MAX_TENANTS] = {0};
Floor floors[MAX_FLOOR_COUNT] = {0};
TempAccess temp_codes[MAX_TEMP_CODES] = {0};
Washroom washrooms[MAX_WASHROOMS] = {0};
uint16_t people_count = 0;
uint16_t people_upstairs = 0;
uint8_t entrance_1_count = 0;
uint8_t entrance_2_count = 0;
uint8_t exit_count = 0;
int last_tenant_id = 0;
float base_rent = 1000.0; // Example base rent

void system_init(void) {
    cli(); // Disable interrupts
    uart_init();
    // bluetooth_init();
    // sensor_init();

    DDRF = 0X01; //provide voltage for sensors of entry and exit
    DDRH = 0xE0; // some ports are input while others are output
    PORTH = 0X1F; 
    lcd_init();
    // lcd_display_number(people_count);

    ESCALATOR_DDR = 0xff;
    ESCALATOR_PORT = 0;

    // Initialize floors & rent calculation
    for (int i = 0; i < MAX_FLOOR_COUNT; i++) {
        int floor_number = i + 1;
        uint16_t rent = base_rent;
        switch (floor_number) {
            case 2:
                rent = base_rent * 0.75;
                break;
            case 3:
                rent = (base_rent * 0.75) * 0.75;
                break;
        }
        floors[i].floor_number = floor_number;
        floors[i].floor_rent = rent;
        floors[i].tenant_count = 0;
    }

    sei(); // Enable interrupts
}

// Function to add a tenant to the system

int add_tenant() {
    uint8_t floor_number = 0;
    while(1){
        lcd_clear();
        lcd_print("Floor:");
        char x =uart_receive();
        floor_number = x - '0';
        if (floor_number < 1 || floor_number > MAX_FLOOR_COUNT) {
            lcd_clear();
            lcd_print("Invalid Floor");
            _delay_ms(1000);
            lcd_clear();
            lcd_print("Try Again");
            _delay_ms(1000);
            continue;
        }
        Floor *floor = &floors[floor_number - 1];
        lcd_clear();
        if (floor->tenant_count >= 10) {
            lcd_print("Floor is Full");
            continue;
        }
        lcd_print("Floor: ");
        lcd_display_number(floor_number);
        _delay_ms(500);
        lcd_clear();
        break;
    }
    Floor *floor = &floors[floor_number - 1];
    // Name
    lcd_print("Name [max=20]:");
    char name[20] = {0};
    int typing=0;
    int index=0;
    while (1) {
        char x = uart_receive();
        if (typing == 0) {
            lcd_clear();
            typing = 1;
        }
        if (index < 19) { // Ensure we don't overflow the buffer
            name[index++] = x;
            name[index] = '\0'; // Null-terminate the string
            lcd_send_data(x);
        }
        if (x == '\0'||x=='\n'||x=='\r') {
            break;
        }
    }
    lcd_clear();
    // Password
    lcd_print("Password [max=6]:");
    char password[CODE_LENGTH + 1] = {0};
    index=0;
    typing = 0;
    while (1) {
        char x = uart_receive();
        if (typing == 0) {
            lcd_clear();
            typing = 1;
        }
        if (index < CODE_LENGTH) { // Ensure we don't overflow the buffer
            password[index++] = x;
            password[index] = '\0'; // Null-terminate the string
            lcd_send_data('*');
        }
        if (x == '\0'||x=='\n'||x=='\r') {
            break;
        }
    }
    lcd_clear();
    tenants[last_tenant_id].id = last_tenant_id;
    tenants[last_tenant_id].floor = floor;
    tenants[last_tenant_id].active = 1;
    strncpy(tenants[last_tenant_id].name, name, sizeof (name));
    strncpy(tenants[last_tenant_id].passcode, password , CODE_LENGTH + 1);
    floor->tenant_count ++;
    last_tenant_id++;
    lcd_print("Tenant added");

    return 0; // No space available
}

uint32_t getTime() {
    return time(0);
}

uint8_t verify_access_code(const char* code) {
    // Check tenant codes
    for (int i = 0; i < MAX_TENANTS; i++) {
        if (tenants[i].active && strcmp(tenants[i].passcode, code) == 0) {
            if (tenants[i].rent_due <= 0) {
                return 1;
            }
            return 0; // Rent due, access denied
        }
    }

    // Check temporary codes
    uint32_t current_time = getTime();
    for (int i = 0; i < MAX_TEMP_CODES; i++) {
        if (temp_codes[i].active &&
                strcmp(temp_codes[i].code, code) == 0 &&
                current_time < temp_codes[i].expiry_time) {
            return 1;
        }
    }

    return 0;
}

void process_entrance_sensor(uint8_t entrance_num) {
    lcd_clear();
    if (entrance_num == 1) {
        entrance_1_count++;
    } else {
        entrance_2_count++;
    }
    people_count++;
    lcd_display_number(people_count);
    lcd_print(" in mall.");
}

void process_exit_sensor(void) {
    lcd_clear();
    if (people_count > 0) {
        people_count--;
        exit_count++;
    }
    lcd_display_number(people_count);
    lcd_print(" in mall.");
}

void generate_access_code(char* buffer) {
    char tmp_code[CODE_LENGTH + 1] = {0};
    for (int i = 0; i < CODE_LENGTH; i++) {
        tmp_code[i] = rand() % 10;
    }
    tmp_code[CODE_LENGTH+1] = '\0';
    strncpy(buffer, tmp_code, sizeof(buffer));
}

void generate_temp_code(uint16_t tenant_id, uint32_t validity_period) {
    // Find free slot for temporary code
    for (int i = 0; i < MAX_TEMP_CODES; i++) {
        if (!temp_codes[i].active) {
            // Generate random code
            generate_access_code(temp_codes[i].code);
            temp_codes[i].tenant_id = tenant_id;
            temp_codes[i].expiry_time = time(0) + validity_period;
            temp_codes[i].active = 1;
            break;
        }
    }
}

void escalator_control(uint8_t enable) {
    // lcd_clear();
    if (enable) {
        ESCALATOR_PORT |= (1 << ESCALATOR_ENABLE);
        lcd_print("Escalator On");
    } else {
        ESCALATOR_PORT &= ~(1 << ESCALATOR_ENABLE);
        lcd_print("Escalator Off");
    }
}
void show_number_Of_tenants(){
    int tenant_count=0;
    for(int i =0;i<MAX_TENANTS;i++){
        if (tenants[i].active) {
            tenant_count++;
        }
    }
    lcd_clear();
    lcd_print("Tenants: ");
    lcd_display_number(tenant_count);
}

void generate_customer_access_code(uint16_t tenant_id, uint32_t validity_period) {
    // Ensure the tenant_id is valid
    if (tenant_id >= MAX_TENANTS || !tenants[tenant_id].active) {
        lcd_clear();
        lcd_print("Invalid Tenant ID");
        _delay_ms(2000);
        return;
    }

    // Find an available slot in the temporary codes array
    for (int i = 0; i < MAX_TEMP_CODES; i++) {
        if (!temp_codes[i].active) {
            // Generate a random 6-digit code
            char temp_code[CODE_LENGTH + 1] = {0};
            for (int j = 0; j < CODE_LENGTH; j++) {
                temp_code[j] = '0' + (rand() % 10);  // Generate digits 0-9
            }
            temp_code[CODE_LENGTH] = '\0';  // Null-terminate the string

            // Store the generated code in the temp_codes array
            strncpy(temp_codes[i].code, temp_code, CODE_LENGTH + 1);
            temp_codes[i].tenant_id = tenant_id;
            temp_codes[i].expiry_time = getTime() + validity_period;
            temp_codes[i].active = 1;

            // Display the code on the LCD for the tenant
            lcd_clear();
            lcd_print("Code for Cust:");
            lcd_set_cursor(1, 0);  // Move to the next line
            lcd_print(temp_code);
            _delay_ms(4000);

            return;
        }
    }

    // No available slot for new temporary code
    lcd_clear();
    lcd_print("No Slots Available");
    _delay_ms(2000);
}


void handle_bluetooth_command(char command) {
    lcd_clear();
    switch (command) {
        case 'e': // Enable escalator
            escalator_control(1);
            break;
        case 'd': // Disable escalator
            escalator_control(0);
            break;
        case 'a':
            add_tenant();
            break;

        case 's':
            show_number_Of_tenants();
            break;
        case 'r': // Ask for rent i.e. Add rent_due to all tenants according to their floors
            lcd_print("Ask Rent? [y/n]");
            char option = uart_receive();
            lcd_clear();
            if (option == 'y') {
                for (int t = 0; t < sizeof(tenants); t++) {
                    if (tenants[t].active) {
                        tenants[t].rent_due += tenants[t].floor->floor_rent;
                    }
                }
                // loop to add rent_due to each tenant
                lcd_print("Rent Added");
            } else {
                lcd_print("Canceled...");
            }
            break;
            // Add more commands as needed
        default:
            lcd_print("No Command!");
    }
}

void update_washroom_status(void) {
    for (int i = 0; i < MAX_WASHROOMS; i++) {
        // Check if washroom is occupied based on sensors
        // Update display status
        if (washrooms[i].occupied) {
            // Display "Occupied" message for this washroom
        }
    }
}

void bluetooth_init() {
}

void sensor_init() {
}
/**
* port must be a PIN reference (e.g. PINn)
* port's data direction must be input (DDRn = 0) 
* port's PORT must be output (PORTn = 0xFF)
* pin must be part of the port (e.g. Pn0)
* where 'n' is any of the ports A,B,C,D e.t.c
*/
int poll(uint8_t pin, uint16_t port) {
    if ((port & 1<<pin) == 0)
        return 1;
    return 0;
}

int main() {
    system_init();
    while (1) {
        int ent1 = poll(PH2, PINH);
        if (ent1) {
            process_entrance_sensor(1);
            while(poll(PH2, PINH));
        }
        int ent2 = poll(PH3, PINH);
        if (ent2) {
            process_entrance_sensor(2);
            while(poll(PH3, PINH));
        }
        int exit1 = poll(PH4, PINH);
        if (exit1) {
            process_exit_sensor();
            while(poll(PH4, PINH));
        }
        
    }
    return 0;
}

ISR(USART0_RX_vect) {
    char received = UDR0;
    handle_bluetooth_command(received);
}
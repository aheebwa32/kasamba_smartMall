// main.c
// Authors: Agani, Conrad, Steve, Samuel
#include "smart_mall.h"
#include "uart.h"
#include "lcd.h"
#include "keypad.h"

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
int last_tenant_id = 1;
float base_rent = 1000.0; // Example base rent

void system_init(void) {
    cli(); // Disable interrupts
    uart_init();
    bluetooth_init();
    sensor_init();

    DDRF = 0X01; //provide voltage for sensors of entry and exit
    DDRH = 0xE0; // some ports are input while others are output
    PORTH = 0X1F; 
    lcd_init();
    // lcd_display_number(people_count);
    keypad_init();

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
        _delay_ms(100);
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
        if (x=='\n'||x=='\r') {
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
        if (x=='\n'||x=='\r') {
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
    _delay_ms(750);
    lcd_clear();
    lcd_print("Kasamba S.M.");

    return 0;
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

void get_random_code(char* buffer) {
    int nums[CODE_LENGTH];
    for (int i = 0; i < CODE_LENGTH; i++) {
        nums[i] = (rand() % 10); // Convert number to character
        // lcd_display_number(nums[i]);
    }
    // _delay_ms(2000);
    for (int i = 0; i < CODE_LENGTH; i++) {
        buffer[i] = nums[i] + '0'; // Convert number to character
    }
    buffer[CODE_LENGTH + 1] = '\0'; // Null-terminate the string
}

TempAccess* generate_temp_code(uint16_t tenant_id, uint32_t validity_period) {
    // Find free slot for temporary code
    for (int i = 0; i < MAX_TEMP_CODES; i++) {
        if (!temp_codes[i].active) {
            // Generate random code
            get_random_code(temp_codes[i].code);
            temp_codes[i].tenant_id = tenant_id;
            temp_codes[i].expiry_time = time(0) + validity_period;
            temp_codes[i].active = 1;
            return &temp_codes[i];
        }
    }
    return NULL;
}

void escalator_control(uint8_t enable) {
    lcd_clear();
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

void process_escalator_sensor(uint8_t direction) {
    lcd_clear();
    if (direction == 1) {
        people_upstairs++;
        // lcd_print("Escalator Up");
    } else {
        if (people_upstairs > 0) {
            people_upstairs--;
        }
        // lcd_print("Escalator Down");
    }
    lcd_print("Upstairs: ");
    lcd_display_number(people_upstairs);
}

Tenant* get_tenant() {
    lcd_print("Enter ID: ");
    char c[4] = {'\0'};
    uint8_t typing = 0;
    uint8_t index = 0;
    while (1) {
        char x = uart_receive();
        if (typing == 0) {
            lcd_clear();
            typing = 1;
        }
        if (x=='\n'||x=='\r') {
            break;
        }
        if (index < 3) { // Ensure we don't overflow the buffer
            c[index++] = x;
            c[index] = '\0'; // Null-terminate the string
            lcd_send_data(x);
        } else break;
    }
    int id = atoi(c);
    if (id < 1) {
        lcd_clear();
        lcd_print("Invalid ID");
        return NULL;
    }
    for (int i = 0; i < MAX_TENANTS; i++) {
        if (tenants[i].active && tenants[i].id == id) {
            return &tenants[i];
        }
    }
    lcd_clear();
    lcd_print("No Tenant Found");
    return NULL;
}

void display_tenant_details(Tenant *tenant) {
    lcd_clear();
    lcd_print("Tenant ID: ");
    lcd_display_number(tenant->id);
    _delay_ms(1000);
    lcd_clear();
    lcd_print("Name: ");
    lcd_print(tenant->name);
    _delay_ms(1000);
    lcd_clear();
    lcd_print("Floor: ");
    lcd_display_number(tenant->floor->floor_number);
    _delay_ms(1000);
    lcd_clear();
    lcd_print("Rent Due: ");
    lcd_display_number(tenant->rent_due);
    _delay_ms(1000);
    lcd_clear();
    lcd_print("Kasamba S.M.");
}

void handle_bluetooth_command(char command) {
    switch (command) {
        case 'e': // Enable escalator
            escalator_control(1);
            break;
        case 'd': // Disable escalator
            escalator_control(0);
            break;
        case 'a':
            lcd_clear();
            add_tenant();
            break;
        case 'g': // Generate temp code
            lcd_clear();
            {
                Tenant *ten = get_tenant();
                if (ten == NULL) {
                    break;
                }
                TempAccess* code_g = generate_temp_code(ten->id, 600);
                lcd_clear();
                if (code_g == NULL) {
                    lcd_print("Generation Failed");
                } else {
                    lcd_print("Code: ");
                    lcd_print(code_g->code);
                }
            }
            break;
        case 'l':
            PORTA ^= (1 << PA2);
            break;
        case 'w':
            lcd_clear2();
            {
                char input[CODE_LENGTH + 1] = {0};
                for (int i = 0; i < CODE_LENGTH; i++) {
                    char key = keypad_scan();
                    if (key == NULL) {
                        i--;
                        _delay_ms(100);
                    }else if (key == '#') {
                        i = -1;
                        lcd_clear2();
                        while (keypad_scan() != NULL);
                    } else {
                        input[i] = key;
                        lcd_send_data2('*');
                        while (keypad_scan() != NULL);
                    }
                    input[i + 1] = '\0';
                }
                lcd_clear2();
                if (verify_access_code(input)) {
                    lcd_print2("Access Granted");
                } else {
                    lcd_print2("Access Denied");
                }
            }
            break;
        case 'v': // View tenant details
            lcd_clear();
            {
                Tenant *ten = get_tenant();
                if (ten == NULL) {
                    break;
                }
                display_tenant_details(ten);
            }
            break;
        case 's':
            lcd_clear();
            show_number_Of_tenants();
            break;
        case 'r': // Ask for rent i.e. Add rent_due to all tenants according to their floors
            lcd_clear();
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
            lcd_clear();
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
    _delay_ms(100);
    uart_send_string("AT\r\n");
    _delay_ms(500);
    uart_send_string("AT+NAME=TheEmporio\r\n");
    _delay_ms(500);
    uart_send_string("AT+PIN=0000\r\n");
    _delay_ms(500);
    uart_send_string("AT+BAUD4\r\n");
    _delay_ms(500);
    uart_send_string("AT+ROLE=0\r\n");
    _delay_ms(500);
    uart_send_string("AT+CMODE=1\r\n");
}

// Function to initialize sensors for the escalator
void sensor_init(void) {
    // Configure ESCALATOR_ENTRY and ESCALATOR_EXIT as input with pull-up resistors
    DDRD &= ~(1 << ESCALATOR_ENTRY);
    DDRD &= ~(1 << ESCALATOR_EXIT);
    PORTD |= (1 << ESCALATOR_ENTRY);
    PORTD |= (1 << ESCALATOR_EXIT);

    // Enable external interrupts for ESCALATOR_ENTRY and ESCALATOR_EXIT
    EICRA |= (1 << ISC01) | (1 << ISC00); // Rising edge on INT0 (PD0)
    EICRA |= (1 << ISC11) | (1 << ISC10); // Rising edge on INT1 (PD1)
    EIMSK |= (1 << INT0) | (1 << INT1);   // Enable INT0 and INT1
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
        int ent1 = poll(MALL_ENTRANCE_1, PINH);
        if (ent1) {
            process_entrance_sensor(1);
            while(poll(MALL_ENTRANCE_1, PINH));
        }
        int ent2 = poll(MALL_ENTRANCE_2, PINH);
        if (ent2) {
            process_entrance_sensor(2);
            while(poll(MALL_ENTRANCE_2, PINH));
        }
        int exit1 = poll(MALL_EXIT, PINH);
        if (exit1) {
            process_exit_sensor();
            while(poll(MALL_EXIT, PINH));
        }
    }
    return 0;
}

ISR(USART0_RX_vect) {
    char received = UDR0;
    handle_bluetooth_command(received);
}

ISR(INT0_vect) {
    people_upstairs++;
    lcd_clear();
    lcd_print("Upstairs: ");
    lcd_display_number(people_upstairs);
}

ISR(INT1_vect) {
    if (people_upstairs > 0) {
        people_upstairs--;
    }
    lcd_clear();
    lcd_print("Upstairs: ");
    lcd_display_number(people_upstairs);
}

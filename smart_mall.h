// smart_mall.h
#ifndef SMART_MALL_H
#define SMART_MALL_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>
#include <math.h>
#include <time.h>

// System Constants
#define MAX_TENANTS 30
#define MAX_TEMP_CODES 20
#define CODE_LENGTH 6
#define MAX_FLOOR_COUNT 3
#define MAX_WASHROOMS 6
#define F_CPU 1000000UL // 1MHz
#define BAUD 19200
#define BAUD_PRESCALER (((F_CPU / (BAUD * 16UL))) - 1)

// Structure Definitions
typedef struct {
    uint8_t floor_number;
    uint8_t tenant_count;
    uint16_t floor_rent;
} Floor;

typedef struct {
    uint16_t id;
    char passcode[CODE_LENGTH + 1];
    Floor *floor;
    float rent_due;
    char name[20];
    uint8_t active;
} Tenant;

typedef struct {
    char code[CODE_LENGTH + 1];
    uint32_t expiry_time;
    uint16_t tenant_id;
    uint8_t active;
} TempAccess;

typedef struct {
    uint8_t occupied;
    uint8_t floor;
    uint32_t last_access_time;
} Washroom;

// Global Variables
extern Tenant tenants[MAX_TENANTS];
extern Floor floors[MAX_FLOOR_COUNT];
extern TempAccess temp_codes[MAX_TEMP_CODES];
extern Washroom washrooms[MAX_WASHROOMS];
extern uint16_t people_count;
extern uint16_t people_upstairs;
extern uint8_t entrance_1_count;
extern uint8_t entrance_2_count;
extern uint8_t exit_count;
extern float base_rent;
extern int last_tenant_id;

// Function Declarations
void system_init(void);
void uart_init(void);
void bluetooth_init(void);
void sensor_init(void);
uint8_t verify_access_code(const char* code);
void update_tenant_rent(uint16_t tenant_id, float amount);
TempAccess* generate_temp_code(uint16_t tenant_id, uint32_t validity_period);
void escalator_control(uint8_t enable);
void process_entrance_sensor(uint8_t entrance_num);
void process_exit_sensor(void);
void handle_bluetooth_command(char command);
void update_washroom_status(void);

// Pin Configurations
#define MALL_ENTRANCE_1 PH2
#define MALL_ENTRANCE_2 PH3
#define MALL_EXIT PH4

#define ESCALATOR_ENTRY PD0
#define ESCALATOR_EXIT PD1
//
#define ESCALATOR_PORT PORTA
#define ESCALATOR_ENABLE PA0
#define ESCALATOR_DDR DDRA



#endif

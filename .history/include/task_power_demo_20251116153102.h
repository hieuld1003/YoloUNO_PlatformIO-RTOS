#ifndef TASK_POWER_DEMO_H
#define TASK_POWER_DEMO_H

#include <Arduino.h>

// Power modes
enum PowerMode {
    MODE_ACTIVE,
    MODE_LIGHT_SLEEP,
    MODE_DEEP_SLEEP
};

// Configuration
#define WAKEUP_BUTTON_PIN   0       // BOOT button (GPIO0)
#define STATUS_LED_PIN      2       // Built-in LED
#define SLEEP_DURATION_SEC  5       // Sleep for 5 seconds

// Function declarations
void task_power_demo_init();
void task_power_management(void *pvParameters);
void enter_light_sleep(uint32_t time_ms);
void enter_deep_sleep(uint32_t time_sec);
void print_power_stats();

#endif
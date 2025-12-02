#ifndef TASK_POWER_DEMO_H
#define TASK_POWER_DEMO_H

#include <Arduino.h>

void task_power_demo_init();
void task_power_management(void *pvParameters);
void enter_light_sleep(uint32_t time_ms);
void enter_deep_sleep(uint32_t time_sec);
void print_power_comparison();

#endif
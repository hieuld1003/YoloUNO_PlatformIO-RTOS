#ifndef LESSON_POWER_OPTIMIZATION_H
#define LESSON_POWER_OPTIMIZATION_H

#include <Arduino.h>

// Sleep mode definitions
#define SLEEP_MODE_LIGHT    1
#define SLEEP_MODE_DEEP     2

// Function prototypes
void taskSensorWithSleep(void *pvParameters);
void taskDeepSleepDemo(void *pvParameters);
void taskPowerComparison(void *pvParameters);

#endif
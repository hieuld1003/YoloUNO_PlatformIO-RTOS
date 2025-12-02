
#include <Arduino.h>
#include "task_power_demo.h"



void task_power_management(void *pvParameters);

void setup() {
    // Khởi tạo Serial ở đây là tốt nhất để debug ngay từ đầu
    Serial.begin(115200);
    
    // Đợi một chút cho Serial ổn định
    delay(1000);
    Serial.println("--- SYSTEM START ---");

    // Tạo Task
    xTaskCreate(
        task_power_management,
        "PowerDemo",
        4096,
        NULL,
        1,
        NULL
    );
}

void loop() {
    // Loop trống để FreeRTOS tự quản lý
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
/**
 * ESP32-S3 Power Optimization Demo
 * Demonstrates: Light Sleep, Deep Sleep, Power Measurement
 * 
 * Hardware needed:
 * - ESP32-S3 YoloUNO
 * - DHT20 sensor (I2C)
 * - LED on GPIO2
 * - Optional: Multimeter to measure current
 */

#include <Arduino.h>
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_pm.h"

// Existing includes
#include "global.h"
#include "temp_humi_monitor.h"
#include "led_blinky.h"

// ==================== CONFIGURATION ====================
#define SLEEP_MODE_LIGHT    1    // Change to 2 for Deep Sleep
#define SLEEP_MODE_DEEP     2

#define CURRENT_SLEEP_MODE  SLEEP_MODE_LIGHT  // ← Change this to test different modes

#define WAKEUP_TIME_SEC     10   // Sleep duration (seconds)
#define LED_PIN             2    // Status LED

// ==================== POWER MEASUREMENT GUIDE ====================
/*
 * HOW TO MEASURE CURRENT:
 * 
 * 1. Hardware Setup:
 *    - Disconnect USB cable
 *    - Connect multimeter in SERIES with VCC (3.3V rail)
 *    - Set multimeter to measure current (mA range)
 * 
 * 2. Expected Current Consumption:
 *    - Active mode (WiFi ON):     150-250 mA
 *    - Active mode (WiFi OFF):    40-80 mA
 *    - Light Sleep:               0.8-3 mA
 *    - Deep Sleep:                10-150 μA (0.01-0.15 mA)
 * 
 * 3. Pin connections for measurement:
 *    Multimeter RED → VCC (3.3V supply)
 *    Multimeter BLACK → ESP32-S3 VCC pin
 */

// ==================== TASK 1: Sensor Reading with Light Sleep ====================
void taskSensorWithSleep(void *pvParameters) {
    float temperature, humidity;
    uint32_t readCount = 0;
    
    Serial.println("[SensorTask] Started - Reading every 10 seconds");
    Serial.println("[SensorTask] Device will enter Light Sleep between readings");
    
    while(1) {
        // === ACTIVE PHASE: Read Sensor ===
        Serial.println("\n========================================");
        Serial.printf("[SensorTask] WAKE UP! Reading #%d\n", ++readCount);
        
        // Blink LED to show activity
        digitalWrite(LED_PIN, HIGH);
        
        // Read DHT20 sensor
        if (read_temp_humi(&temperature, &humidity)) {
            Serial.printf("[SensorTask] Temperature: %.1f°C\n", temperature);
            Serial.printf("[SensorTask] Humidity: %.1f%%\n", humidity);
        } else {
            Serial.println("[SensorTask] Sensor read failed!");
        }
        
        digitalWrite(LED_PIN, LOW);
        
        // === SLEEP PHASE ===
        Serial.printf("[SensorTask] Entering Light Sleep for %d seconds...\n", 
                      WAKEUP_TIME_SEC);
        Serial.println("========================================\n");
        Serial.flush();  // Wait for Serial to finish
        
        // Configure wakeup timer
        esp_sleep_enable_timer_wakeup(WAKEUP_TIME_SEC * 1000000ULL);  // microseconds
        
        // Enter Light Sleep (WiFi, CPU sleep, but RAM retained)
        esp_light_sleep_start();
        
        // Device wakes up here after timer expires
        Serial.println("[SensorTask] Woke up from Light Sleep!");
    }
}

// ==================== TASK 2: Deep Sleep Demo ====================
void taskDeepSleepDemo(void *pvParameters) {
    uint32_t bootCount = 0;
    
    // Check wakeup reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[DeepSleep] Woke up from timer");
            bootCount++;
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[DeepSleep] Woke up from external signal");
            break;
        default:
            Serial.println("[DeepSleep] First boot / Not from deep sleep");
            break;
    }
    
    // Blink LED 3 times to show we're awake
    for(int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    // Read sensor one time
    float temperature, humidity;
    if (read_temp_humi(&temperature, &humidity)) {
        Serial.printf("[DeepSleep] Temp: %.1f°C, Humidity: %.1f%%\n", 
                      temperature, humidity);
    }
    
    // Print statistics
    Serial.printf("[DeepSleep] Boot count: %d\n", bootCount);
    Serial.printf("[DeepSleep] Entering Deep Sleep for %d seconds...\n", 
                  WAKEUP_TIME_SEC);
    Serial.println("*** DEVICE WILL RESET ON WAKEUP ***\n");
    Serial.flush();
    
    // Configure wakeup
    esp_sleep_enable_timer_wakeup(WAKEUP_TIME_SEC * 1000000ULL);
    
    // Enter Deep Sleep (CPU OFF, RAM cleared, only RTC memory retained)
    esp_deep_sleep_start();
    
    // ⚠️ CODE NEVER REACHES HERE - Device resets on wakeup!
}

// ==================== TASK 3: Power Comparison Demo ====================
void taskPowerComparison(void *pvParameters) {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   ESP32-S3 Power Consumption Test     ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    // Phase 1: Active mode with WiFi OFF
    Serial.println("PHASE 1: Active Mode (WiFi OFF)");
    Serial.println("→ Connect multimeter now and note current");
    Serial.println("→ Expected: 40-80 mA\n");
    for(int i = 5; i > 0; i--) {
        Serial.printf("   Measuring in %d seconds...\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Busy work to simulate active CPU
    Serial.println("   ⚡ CPU doing work...");
    uint32_t result = 0;
    for(int i = 0; i < 1000000; i++) {
        result += i;
    }
    Serial.printf("   Calculation result: %u\n", result);
    Serial.println("   ✓ Measurement complete!\n");
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Phase 2: Light Sleep
    Serial.println("PHASE 2: Light Sleep Mode");
    Serial.println("→ Note the LOWER current draw");
    Serial.println("→ Expected: 0.8-3 mA\n");
    for(int i = 5; i > 0; i--) {
        Serial.printf("   Entering sleep in %d seconds...\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    Serial.println("   💤 Entering Light Sleep for 10 seconds...");
    Serial.flush();
    
    esp_sleep_enable_timer_wakeup(10 * 1000000ULL);
    esp_light_sleep_start();
    
    Serial.println("   ✓ Woke up from Light Sleep!\n");
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Phase 3: Instructions for Deep Sleep test
    Serial.println("PHASE 3: Deep Sleep Test");
    Serial.println("→ Change CURRENT_SLEEP_MODE to SLEEP_MODE_DEEP");
    Serial.println("→ Re-upload code");
    Serial.println("→ Expected: 10-150 μA (0.01-0.15 mA)");
    Serial.println("→ Device will RESET every 10 seconds\n");
    
    // Hold here
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ==================== MAIN SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize I2C for DHT20
    init_temp_humi_sensor();
    
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  ESP32-S3 Power Optimization Demo     ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    #if CURRENT_SLEEP_MODE == SLEEP_MODE_LIGHT
        Serial.println("\n[Mode] LIGHT SLEEP");
        Serial.println("[Info] Device sleeps between sensor readings");
        Serial.println("[Info] RAM is retained, quick wakeup\n");
        
        // Create sensor task with light sleep
        xTaskCreate(
            taskSensorWithSleep,
            "SensorSleep",
            4096,
            NULL,
            1,
            NULL
        );
        
    #elif CURRENT_SLEEP_MODE == SLEEP_MODE_DEEP
        Serial.println("\n[Mode] DEEP SLEEP");
        Serial.println("[Info] Device RESETS on wakeup");
        Serial.println("[Info] Lowest power consumption\n");
        
        // Run deep sleep demo (single task)
        xTaskCreate(
            taskDeepSleepDemo,
            "DeepSleep",
            4096,
            NULL,
            1,
            NULL
        );
        
    #else
        // Power comparison demo
        xTaskCreate(
            taskPowerComparison,
            "PowerTest",
            4096,
            NULL,
            1,
            NULL
        );
    #endif
}

void loop() {
    // Empty - tasks handle everything
    vTaskDelay(pdMS_TO_TICKS(1000));
}
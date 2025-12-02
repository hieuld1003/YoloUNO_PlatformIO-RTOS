#include "task_power_demo.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_pm.h"

// RTC memory to persist across deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint32_t totalSleepTime = 0;

// Current power mode
PowerMode currentMode = MODE_ACTIVE;

// ==================== Print Wake-up Reason ====================
void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    Serial.println("\n========== WAKE UP INFO ==========");
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("⏰ Woke up by: BUTTON (EXT0)");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("⏰ Woke up by: TIMER");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("⏰ Woke up by: TOUCHPAD");
            break;
        default:
            Serial.println("⏰ Woke up by: POWER ON / RESET");
            bootCount = 0;  // Reset boot count on power-on
            break;
    }
    
    Serial.printf("Boot count: %d\n", ++bootCount);
    Serial.printf("Total sleep time: %d seconds\n", totalSleepTime);
    Serial.println("==================================\n");
}

// ==================== Print Power Statistics ====================
void print_power_stats() {
    Serial.println("\n========== POWER STATISTICS ==========");
    Serial.printf("CPU Frequency: %d MHz\n", getCpuFrequencyMhz());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    
    // Read current (if possible - conceptual)
    Serial.println("\n📊 Estimated Current Consumption:");
    Serial.println("  Active Mode:       ~80-160 mA (WiFi off)");
    Serial.println("  Light Sleep:       ~0.8-3 mA");
    Serial.println("  Deep Sleep:        ~10-150 µA");
    Serial.println("======================================\n");
}

// ==================== Light Sleep Mode ====================
void enter_light_sleep(uint32_t time_ms) {
    Serial.printf("\n💤 Entering Light Sleep for %d ms...\n", time_ms);
    Serial.println("   (WiFi/Bluetooth OFF, CPU halted, RTC ON)");
    Serial.flush();
    
    // Configure wake-up sources
    esp_sleep_enable_timer_wakeup(time_ms * 1000);  // Convert ms to µs
    
    // Optional: Wake up by button press (GPIO0)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Wake when button pressed (LOW)
    
    // Turn off LED before sleep
    digitalWrite(STATUS_LED_PIN, LOW);
    
    // Enter light sleep
    uint32_t startTime = millis();
    esp_light_sleep_start();
    uint32_t sleepDuration = millis() - startTime;
    
    // After wake up
    Serial.printf("✅ Woke up after %d ms\n\n", sleepDuration);
    
    // Blink LED to indicate wake-up
    for(int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(100);
        digitalWrite(STATUS_LED_PIN, LOW);
        delay(100);
    }
}

// ==================== Deep Sleep Mode ====================
void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("\n😴 Entering Deep Sleep for %d seconds...\n", time_sec);
    Serial.println("   (Everything OFF except RTC)");
    Serial.println("   Press BOOT button or wait for timer to wake up");
    Serial.flush();
    
    // Update RTC memory
    totalSleepTime += time_sec;
    
    // Configure wake-up sources
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);  // Convert to µs
    
    // Wake up by button (GPIO0) - RTC GPIO required
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Wake on LOW
    
    // Turn off LED
    digitalWrite(STATUS_LED_PIN, LOW);
    
    // Enter deep sleep (DOES NOT RETURN - restarts from setup())
    esp_deep_sleep_start();
}

// ==================== Initialize Power Demo ====================
void task_power_demo_init() {
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(WAKEUP_BUTTON_PIN, INPUT_PULLUP);
    
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  ESP32-S3 Power Optimization Demo     ║");
    Serial.println("║  YOLO UNO Board                        ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    print_wakeup_reason();
    print_power_stats();
    
    Serial.println("\n📝 MENU:");
    Serial.println("  1 - Enter Light Sleep (5 sec)");
    Serial.println("  2 - Enter Deep Sleep (10 sec)");
    Serial.println("  3 - Print Power Stats");
    Serial.println("  4 - Reduce CPU Frequency (power save)");
    Serial.println("  5 - Normal CPU Frequency");
    Serial.println("  b - Press BOOT button to wake from sleep");
    Serial.println("\nSend command via Serial Monitor:");
}

// ==================== Task: Power Management Demo ====================
void task_power_management(void *pvParameters) {
    task_power_demo_init();
    
    uint32_t lightSleepInterval = 10000;  // Enter light sleep every 10s
    uint32_t lastLightSleep = millis();
    
    while(1) {
        // Check Serial commands
        if (Serial.available()) {
            char cmd = Serial.read();
            
            switch(cmd) {
                case '1':
                    Serial.println("\n→ User selected: Light Sleep");
                    enter_light_sleep(5000);  // 5 seconds
                    break;
                    
                case '2':
                    Serial.println("\n→ User selected: Deep Sleep");
                    Serial.println("⚠️  Device will restart after wake-up!");
                    delay(1000);
                    enter_deep_sleep(10);  // 10 seconds
                    break;
                    
                case '3':
                    print_power_stats();
                    break;
                    
                case '4':
                    Serial.println("\n→ Reducing CPU to 80 MHz (power save)");
                    setCpuFrequencyMhz(80);
                    print_power_stats();
                    break;
                    
                case '5':
                    Serial.println("\n→ Restoring CPU to 240 MHz");
                    setCpuFrequencyMhz(240);
                    print_power_stats();
                    break;
                    
                default:
                    Serial.println("❌ Invalid command!");
                    break;
            }
        }
        
        // Auto light sleep demo (optional)
        if (millis() - lastLightSleep > lightSleepInterval) {
            Serial.println("\n🔄 Auto Light Sleep Demo (every 10s)");
            enter_light_sleep(3000);
            lastLightSleep = millis();
        }
        
        // Heartbeat LED (active mode indicator)
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

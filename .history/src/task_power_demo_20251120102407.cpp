#include "task_power_demo.h"
#include "esp_sleep.h"
#include "esp_pm.h"
#include "driver/gpio.h"

// LED Pin (GPIO2 trên YOLO UNO)
#define LED_PIN GPIO_NUM_2

// Boot counter (persists across deep sleep)
RTC_DATA_ATTR int bootCount = 0;

// ==================== Print Wake-up Reason ====================
void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    Serial.println("\n========== WAKE UP INFO ==========");
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("⏰ Woke up by: BUTTON (BOOT)");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("⏰ Woke up by: TIMER");
            break;
        default:
            Serial.println("⏰ Power ON / RESET");
            bootCount = 0;
            break;
    }
    Serial.printf("Boot count: %d\n", bootCount);
    Serial.println("==================================\n");
}

// ==================== Light Sleep (REAL) ====================
void enter_light_sleep(uint32_t time_ms) {
    Serial.printf("\n💤 Entering Light Sleep for %d ms...\n", time_ms);
    Serial.println("   📉 Power: ~120 mA → ~2 mA");
    Serial.println("   💡 LED: Turning OFF...");
    Serial.println("   ⏰ Wake-up sources:");
    Serial.printf("      - Timer (%d ms)\n", time_ms);
    Serial.println("      - BOOT button (GPIO0)");
    Serial.flush();  // Wait for Serial to finish
    
    // Configure wake-up sources
    esp_sleep_enable_timer_wakeup(time_ms * 1000);  // microseconds
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);    // BOOT button (LOW = pressed)
    
    // Turn OFF LED before sleep
    gpio_set_level(LED_PIN, 0);
    
    // ⚡ ENTER LIGHT SLEEP - REAL HARDWARE
    uint32_t start_time = millis();
    esp_light_sleep_start();
    uint32_t sleep_duration = millis() - start_time;
    
    // Turn ON LED after wake
    gpio_set_level(LED_PIN, 1);
    
    // Check wake-up cause
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    
    Serial.printf("\n✅ Woke up after %d ms\n", sleep_duration);
    Serial.println("   📈 Power: ~2 mA → ~120 mA");
    Serial.println("   💡 LED: ON");
    
    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("   🔘 Wake source: BOOT BUTTON");
    } else {
        Serial.println("   ⏰ Wake source: TIMER");
    }
    Serial.println();
}

// ==================== Deep Sleep (REAL) ====================
void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("\n😴 Entering Deep Sleep for %d seconds...\n", time_sec);
    Serial.println("   📉 Power: ~120 mA → ~50 µA");
    Serial.println("   ⚠️  Device will RESTART after wake-up!");
    Serial.println("   💡 LED: Turning OFF...");
    Serial.println("   ⏰ Wake-up sources:");
    Serial.printf("      - Timer (%d seconds)\n", time_sec);
    Serial.println("      - BOOT button (GPIO0)");
    Serial.println("\n   💤 Entering sleep NOW...");
    Serial.flush();
    
    // Save boot count before deep sleep
    bootCount++;
    
    // Configure wake-up sources
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);  // microseconds
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    // Turn OFF LED before sleep
    gpio_set_level(LED_PIN, 0);
    
    // ⚡ ENTER DEEP SLEEP - DEVICE WILL RESTART
    esp_deep_sleep_start();
    
    // ❌ Code below NEVER executes (device restarts)
}

// ==================== Power Comparison ====================
void print_power_comparison() {
    Serial.println("\n========== POWER COMPARISON ==========");
    Serial.println("📦 Battery: 500mAh LiPo (3.7V)\n");
    Serial.println("┌──────────────┬──────────┬──────────┬─────────┐");
    Serial.println("│ Mode         │ Current  │ Runtime  │ Savings │");
    Serial.println("├──────────────┼──────────┼──────────┼─────────┤");
    Serial.println("│ Active       │ ~100 mA  │ 5 hours  │ 1x      │");
    Serial.println("│ Light Sleep  │ ~2 mA    │ 250 hrs  │ 50x     │");
    Serial.println("│ Deep Sleep   │ ~0.05 mA │ 10000hrs │ 2000x   │");
    Serial.println("└──────────────┴──────────┴──────────┴─────────┘");
    
    Serial.println("\n💡 Power Consumption Details:");
    Serial.println("   Active: WiFi/BLE OFF, CPU 240MHz, Full peripherals");
    Serial.println("   Light:  WiFi/BLE OFF, CPU halted, RAM retained");
    Serial.println("   Deep:   Everything OFF except RTC timer");
    
    Serial.println("\n📊 Real measurements (you can verify):");
    Serial.println("   1. Connect USB power meter (KWS-MX18)");
    Serial.println("   2. Observe current draw in each mode");
    Serial.println("   3. Active: LED ON, Serial active");
    Serial.println("   4. Sleep:  LED OFF, minimal power");
    
    Serial.println("======================================\n");
}

// ==================== Init ====================
void task_power_demo_init() {
    Serial.begin(115200);
    delay(1000);
    
    // Configure LED pin
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 1);  // LED ON = Active
    
    Serial.println("\n\n╔════════════════════════════════════╗");
    Serial.println("║  Power Optimization Demo          ║");
    Serial.println("║  YOLO UNO - REAL HARDWARE MODE    ║");
    Serial.println("╚════════════════════════════════════╝\n");
    
    // Print wake-up reason
    print_wakeup_reason();
    
    Serial.printf("🔌 CPU Frequency: %d MHz\n", getCpuFrequencyMhz());
    Serial.printf("💾 Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("🔋 Chip: %s Rev %d\n\n", ESP.getChipModel(), ESP.getChipRevision());
    
    print_power_comparison();
    
    Serial.println("📝 INTERACTIVE MENU:");
    Serial.println("┌────┬──────────────────────────────────┐");
    Serial.println("│ 1  │ Light Sleep (5s) - REAL          │");
    Serial.println("│ 2  │ Deep Sleep (10s) - REAL RESTART  │");
    Serial.println("│ 3  │ Show Power Comparison Table      │");
    Serial.println("└────┴──────────────────────────────────┘\n");
    
    Serial.println("💡 LED Indicators:");
    Serial.println("   🔴 LED ON  = Active mode (~100mA)");
    Serial.println("   ⚫ LED OFF = Sleep mode (~2mA or ~50µA)");
    
    Serial.println("\n⚠️  IMPORTANT:");
    Serial.println("   - Deep Sleep will RESTART the board");
    Serial.println("   - Press BOOT button to wake early");
    Serial.println("   - Observe LED: OFF = sleeping\n");
    
    Serial.println(">>> Type a command (1, 2, 3):\n");
}

// ==================== Main Task ====================
void task_power_management(void *pvParameters) {
    task_power_demo_init();
    
    while(1) {
        if (Serial.available()) {
            char cmd = Serial.read();
            while(Serial.available()) Serial.read();  // Clear buffer
            
            if (cmd == '\n' || cmd == '\r') {
                continue;  // Ignore newlines
            }
            
            Serial.printf("\n🎯 Command: '%c'\n", cmd);
            Serial.println("═══════════════════════════════════════");
            
            switch(cmd) {
                case '1':
                    enter_light_sleep(5000);
                    break;
                    
                case '2':
                    enter_deep_sleep(10);
                    break;
                    
                case '3':
                    print_power_comparison();
                    break;
                    
                default:
                    Serial.printf("❌ Unknown command: '%c'\n", cmd);
                    Serial.println("💡 Valid: 1, 2, 3\n");
                    break;
            }
            
            if (cmd >= '1' && cmd <= '3') {
                Serial.println("═══════════════════════════════════════");
                Serial.println(">>> Type next command:\n");
            }
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
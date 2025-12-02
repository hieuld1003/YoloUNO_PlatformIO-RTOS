#include "task_power_demo.h"
#include "esp_sleep.h"
#include "esp_pm.h"

// RTC memory to persist across deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint32_t totalSleepTime = 0;

// Simulation mode (set to false when you have hardware)
#define SIMULATION_MODE true

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
            bootCount = 0;
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
    
    Serial.println("\n📊 Estimated Current Consumption:");
    Serial.println("┌─────────────────┬──────────────┬──────────────┐");
    Serial.println("│ Mode            │ 240 MHz      │ 80 MHz       │");
    Serial.println("├─────────────────┼──────────────┼──────────────┤");
    Serial.println("│ Active (no WiFi)│ ~80-120 mA   │ ~40-60 mA    │");
    Serial.println("│ Light Sleep     │ ~0.8-3 mA    │ ~0.8-3 mA    │");
    Serial.println("│ Deep Sleep      │ ~10-150 µA   │ ~10-150 µA   │");
    Serial.println("└─────────────────┴──────────────┴──────────────┘");
    Serial.println("======================================\n");
}

// ==================== Light Sleep ====================
void enter_light_sleep(uint32_t time_ms) {
    if (SIMULATION_MODE) {
        Serial.printf("\n💤 [SIMULATION] Light Sleep for %d ms\n", time_ms);
        Serial.println("   Real behavior: WiFi/BT OFF, CPU halted, RTC ON");
        Serial.println("   Simulating sleep with vTaskDelay()...");
        Serial.flush();
        
        Serial.println("\n   📉 Power consumption: 120 mA → 2 mA");
        Serial.println("   💾 RAM retained: YES");
        Serial.println("   📡 Peripherals: DISABLED");
        
        vTaskDelay(time_ms / portTICK_PERIOD_MS);
        
        Serial.printf("\n✅ [SIMULATION] Woke up after %d ms\n", time_ms);
        Serial.println("   📈 Power consumption: 2 mA → 120 mA\n");
        
        return;
    }
    
    // Real implementation
    Serial.printf("\n💤 Entering Light Sleep for %d ms...\n", time_ms);
    Serial.flush();
    
    esp_sleep_enable_timer_wakeup(time_ms * 1000);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    digitalWrite(STATUS_LED_PIN, LOW);
    
    uint32_t startTime = millis();
    esp_light_sleep_start();
    uint32_t sleepDuration = millis() - startTime;
    
    Serial.printf("✅ Woke up after %d ms\n\n", sleepDuration);
}

// ==================== Deep Sleep ====================
void enter_deep_sleep(uint32_t time_sec) {
    if (SIMULATION_MODE) {
        Serial.printf("\n😴 [SIMULATION] Deep Sleep for %d seconds\n", time_sec);
        Serial.println("   Real behavior: Everything OFF except RTC");
        Serial.println("   Device would RESTART after wake-up");
        Serial.println("\n   Simulating power states:");
        
        Serial.println("\n   ⚡ Power sequence:");
        Serial.println("   1. CPU: 240 MHz → OFF");
        Serial.println("   2. WiFi: ON → OFF");
        Serial.println("   3. RAM: Powered → RTC only");
        Serial.println("   4. Current: 120 mA → 50 µA");
        
        for(int i = time_sec; i > 0; i--) {
            Serial.printf("   💤 Sleeping... %d seconds remaining\n", i);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        
        totalSleepTime += time_sec;
        
        Serial.println("\n   🔌 [SIMULATION] Wake-up trigger!");
        Serial.println("   ⚡ Power sequence:");
        Serial.println("   1. RTC wakeup signal");
        Serial.println("   2. CPU: OFF → Bootloader");
        Serial.println("   3. RAM: RTC → Full power");
        Serial.println("   4. Current: 50 µA → 120 mA");
        Serial.printf("\n✅ [SIMULATION] Would restart now (boot count would be %d)\n\n", bootCount + 1);
        
        return;
    }
    
    // Real implementation
    Serial.printf("\n😴 Entering Deep Sleep for %d seconds...\n", time_sec);
    Serial.flush();
    
    totalSleepTime += time_sec;
    
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    digitalWrite(STATUS_LED_PIN, LOW);
    
    esp_deep_sleep_start();
}

// ==================== Calculate Power Savings ====================
void calculate_power_savings() {
    Serial.println("\n========== POWER SAVINGS CALCULATOR ==========");
    
    float active_current_ma = 100.0;
    float light_sleep_current_ma = 2.0;
    float deep_sleep_current_ua = 50.0;
    float battery_mah = 500.0;
    
    Serial.println("\n📦 Scenario: 500mAh battery");
    Serial.println("┌──────────────────┬─────────────┬──────────────┐");
    Serial.println("│ Mode             │ Runtime     │ Savings      │");
    Serial.println("├──────────────────┼─────────────┼──────────────┤");
    
    float active_hours = battery_mah / active_current_ma;
    Serial.printf("│ Always Active    │ %.1f hours  │ Baseline     │\n", active_hours);
    
    float light_avg = (0.1 * active_current_ma) + (0.9 * light_sleep_current_ma);
    float light_hours = battery_mah / light_avg;
    Serial.printf("│ 90%% Light Sleep │ %.1f hours │ %.0fx longer │\n", 
                  light_hours, light_hours / active_hours);
    
    float deep_avg_ma = (0.1 * active_current_ma) + (0.9 * deep_sleep_current_ua / 1000.0);
    float deep_hours = battery_mah / deep_avg_ma;
    Serial.printf("│ 90%% Deep Sleep  │ %.1f hours│ %.0fx longer │\n", 
                  deep_hours, deep_hours / active_hours);
    
    Serial.println("└──────────────────┴─────────────┴──────────────┘");
    Serial.println("==============================================\n");
}

// ==================== CPU Frequency Test ====================
void test_cpu_frequency() {
    Serial.println("\n========== CPU FREQUENCY TEST ==========");
    
    uint32_t frequencies[] = {240, 160, 80, 40, 20, 10};
    
    for(int i = 0; i < 6; i++) {
        setCpuFrequencyMhz(frequencies[i]);
        Serial.printf("\n📊 Testing at %d MHz:\n", frequencies[i]);
        Serial.printf("   Actual CPU Freq: %d MHz\n", getCpuFrequencyMhz());
        
        uint32_t start = micros();
        volatile uint32_t sum = 0;
        for(int j = 0; j < 100000; j++) {
            sum += j;
        }
        uint32_t duration = micros() - start;
        
        Serial.printf("   100K iterations: %d µs\n", duration);
        Serial.printf("   Est. current: ~%.0f mA\n", 120.0 * (frequencies[i] / 240.0));
        
        delay(1000);
    }
    
    setCpuFrequencyMhz(240);
    Serial.println("\n✅ Restored to 240 MHz");
    Serial.println("======================================\n");
}

// ==================== Initialize ====================
void task_power_demo_init() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  ESP32-S3 Power Optimization Demo     ║");
    Serial.println("║  YOLO UNO Board - SIMULATION MODE     ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    if (SIMULATION_MODE) {
        Serial.println("\n⚠️  SIMULATION MODE ENABLED");
        Serial.println("   - No hardware required");
        Serial.println("   - Sleep functions are mocked");
        Serial.println("   - Power values are estimated\n");
    }
    
    print_wakeup_reason();
    print_power_stats();
    
    Serial.println("\n📝 INTERACTIVE MENU:");
    Serial.println("┌────┬─────────────────────────────────────┐");
    Serial.println("│ 1  │ Light Sleep Test (5 sec)            │");
    Serial.println("│ 2  │ Deep Sleep Test (10 sec)            │");
    Serial.println("│ 3  │ Print Power Stats                   │");
    Serial.println("│ 4  │ CPU Frequency Test (all speeds)     │");
    Serial.println("│ 5  │ Power Savings Calculator            │");
    Serial.println("│ 6  │ Set CPU to 80 MHz (power save)      │");
    Serial.println("│ 7  │ Set CPU to 240 MHz (normal)         │");
    Serial.println("│ q  │ Quit / Reset                        │");
    Serial.println("└────┴─────────────────────────────────────┘");
    Serial.println("\n💡 Type a command and press ENTER:");
}

// ==================== Main Task ====================
void task_power_management(void *pvParameters) {
    task_power_demo_init();
    
    while(1) {
        if (Serial.available()) {
            char cmd = Serial.read();
            
            while(Serial.available()) Serial.read();
            
            Serial.printf("\n>>> Command received: '%c'\n", cmd);
            
            switch(cmd) {
                case '1':
                    Serial.println("─────────────────────────────────────");
                    enter_light_sleep(5000);
                    Serial.println("─────────────────────────────────────");
                    break;
                    
                case '2':
                    Serial.println("─────────────────────────────────────");
                    enter_deep_sleep(10);
                    Serial.println("─────────────────────────────────────");
                    break;
                    
                case '3':
                    print_power_stats();
                    break;
                    
                case '4':
                    test_cpu_frequency();
                    break;
                    
                case '5':
                    calculate_power_savings();
                    break;
                    
                case '6':
                    Serial.println("\n→ Reducing CPU to 80 MHz");
                    setCpuFrequencyMhz(80);
                    Serial.printf("   ✅ New frequency: %d MHz\n", getCpuFrequencyMhz());
                    Serial.println("   💡 Power saving: ~50%%\n");
                    break;
                    
                case '7':
                    Serial.println("\n→ Restoring CPU to 240 MHz");
                    setCpuFrequencyMhz(240);
                    Serial.printf("   ✅ New frequency: %d MHz\n", getCpuFrequencyMhz());
                    Serial.println("   ⚡ Full performance mode\n");
                    break;
                    
                case 'q':
                case 'Q':
                    Serial.println("\n🔄 Resetting menu...\n");
                    task_power_demo_init();
                    break;
                    
                case '\n':
                case '\r':
                    break;
                    
                default:
                    Serial.printf("❌ Unknown command: '%c'\n", cmd);
                    Serial.println("💡 Type a number (1-7) or 'q'\n");
                    break;
            }
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
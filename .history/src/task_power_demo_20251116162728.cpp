#include "task_power_demo.h"
#include "esp_sleep.h"

// LED Pin
#define LED_PIN 2

// Boot counter (persists across deep sleep)
RTC_DATA_ATTR int bootCount = 0;

// ==================== Light Sleep ====================
void enter_light_sleep(uint32_t time_ms) {
    Serial.printf("\n💤 Light Sleep for %d ms\n", time_ms);
    Serial.println("   Power: 120 mA → 2 mA");
    
    // LED OFF = Sleep
    digitalWrite(LED_PIN, LOW);
    delay(time_ms);
    
    // LED ON = Wake up
    digitalWrite(LED_PIN, HIGH);
    Serial.println("✅ Woke up!\n");
}

// ==================== Deep Sleep ====================
void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("\n😴 Deep Sleep for %d seconds\n", time_sec);
    Serial.println("   Power: 120 mA → 50 µA");
    Serial.println("   Device will RESTART after wake\n");
    
    // LED OFF = Sleep
    digitalWrite(LED_PIN, LOW);
    
    // Countdown
    for(int i = time_sec; i > 0; i--) {
        Serial.printf("   💤 %d...\n", i);
        delay(1000);
    }
    
    Serial.println("✅ Done (would restart in real mode)\n");
    digitalWrite(LED_PIN, HIGH);
}

// ==================== Power Comparison ====================
void print_power_comparison() {
    Serial.println("\n========== POWER COMPARISON ==========");
    Serial.println("Battery: 500mAh\n");
    Serial.println("┌──────────────┬──────────┬──────────┐");
    Serial.println("│ Mode         │ Current  │ Runtime  │");
    Serial.println("├──────────────┼──────────┼──────────┤");
    Serial.println("│ Active       │ 100 mA   │ 5 hours  │");
    Serial.println("│ Light Sleep  │ 2 mA     │ 250 hrs  │");
    Serial.println("│ Deep Sleep   │ 0.05 mA  │ 10000hrs │");
    Serial.println("└──────────────┴──────────┴──────────┘");
    Serial.println("======================================\n");
}

// ==================== Init ====================
void task_power_demo_init() {
    Serial.begin(115200);
    delay(1000);
    
    // Setup LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // ON = Active
    
    Serial.println("\n╔══════════════════════════════╗");
    Serial.println("║  Power Optimization Demo    ║");
    Serial.println("╚══════════════════════════════╝");
    
    Serial.printf("Boot count: %d\n", ++bootCount);
    
    print_power_comparison();
    
    Serial.println("📝 MENU:");
    Serial.println("  1 - Light Sleep (5s)");
    Serial.println("  2 - Deep Sleep (10s)");
    Serial.println("  3 - Show Power Table\n");
    
    Serial.println("💡 LED: ON=Active, OFF=Sleep\n");
}

// ==================== Main Loop ====================
void task_power_management(void *pvParameters) {
    task_power_demo_init();
    
    while(1) {
        if (Serial.available()) {
            char cmd = Serial.read();
            while(Serial.available()) Serial.read();
            
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
                    
                case '\n':
                case '\r':
                    break;
                    
                default:
                    Serial.printf("Unknown: '%c'\n", cmd);
            }
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
#include "task_power_demo.h"
#include "esp_sleep.h"
#include "driver/gpio.h"

// LED Pin
#define LED_PIN GPIO_NUM_48

// Boot counter
RTC_DATA_ATTR int bootCount = 0;

// Input buffer
String inputBuffer = "";

// ==================== Print Menu ====================
void print_menu() {
    Serial.println("\n╔═══════════════════════════════╗");
    Serial.println("║  Power Optimization Demo     ║");
    Serial.println("╚═══════════════════════════════╝");
    
    // Wake-up info
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("🔄 Woke by TIMER");
    } else if (wakeup == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("🔄 Woke by BUTTON");
    } else {
        Serial.println("🔄 Power ON");
        bootCount = 0;
    }
    Serial.printf("Boot count: %d\n", bootCount);
    
    Serial.println("\nCOMMANDS:");
    Serial.println("  L <sec> - Light Sleep");
    Serial.println("  D <sec> - Deep Sleep");
    Serial.println("  M       - Show Menu");
    Serial.println("\nEXAMPLES:");
    Serial.println("  L 5     → Light 5s");
    Serial.println("  D 10    → Deep 10s");
    Serial.println("  L 30    → Light 30s");
    Serial.println("\nLED: ON=Active, OFF=Sleep");
    Serial.println("Press Enter after command\n");
    Serial.print(">>> ");
}

// ==================== Light Sleep ====================
void enter_light_sleep(uint32_t time_sec) {
    Serial.printf("\n💤 Light Sleep: %d sec\n", time_sec);
    Serial.println("LED: OFF | Power: 2mA");
    Serial.flush();
    
    gpio_set_level(LED_PIN, 0);
    
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    uint32_t start = millis();
    esp_light_sleep_start();
    uint32_t duration = (millis() - start) / 1000;
    
    gpio_set_level(LED_PIN, 1);
    
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.printf("✅ Woke by BUTTON (%ds)\n", duration);
    } else {
        Serial.printf("✅ Woke by TIMER (%ds)\n", duration);
    }
    Serial.print(">>> ");
}

// ==================== Deep Sleep ====================
void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("\n😴 Deep Sleep: %d sec\n", time_sec);
    Serial.println("LED: OFF | Power: 50µA");
    Serial.println("⚠️  Board will RESTART!\n");
    Serial.flush();
    
    bootCount++;
    
    gpio_set_level(LED_PIN, 0);
    
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    esp_deep_sleep_start();
}

// ==================== Parse Command ====================
void parse_command(String cmd) {
    cmd.trim();
    cmd.toUpperCase();
    
    if (cmd.length() == 0) {
        Serial.print(">>> ");
        return;
    }
    
    // Show menu
    if (cmd == "M") {
        print_menu();
        return;
    }
    
    // Parse L/D command
    if (cmd.length() < 3) {
        Serial.println("❌ Format: L <sec> or D <sec>");
        Serial.print(">>> ");
        return;
    }
    
    char mode = cmd.charAt(0);
    String timeStr = cmd.substring(2);
    timeStr.trim();
    int time = timeStr.toInt();
    
    if (time <= 0 || time > 3600) {
        Serial.println("❌ Time: 1-3600 sec");
        Serial.print(">>> ");
        return;
    }
    
    if (mode == 'L') {
        enter_light_sleep(time);
    } else if (mode == 'D') {
        enter_deep_sleep(time);
    } else {
        Serial.println("❌ Use L or D");
        Serial.print(">>> ");
    }
}

// ==================== Init ====================
void task_power_demo_init() {
    Serial.begin(115200);
    delay(2000);  // Đợi Serial Monitor mở
    
    // Clear buffer
    while(Serial.available()) {
        Serial.read();
    }
    
    // Configure LED
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 1);
    
    print_menu();
}

// ==================== Main Task ====================
void task_power_management(void *pvParameters) {
    task_power_demo_init();
    
    while(1) {
        while (Serial.available()) {
            char c = Serial.read();
            
            // Echo character (hiện ký tự đang gõ)
            if (c >= 32 && c <= 126) {  // Printable characters
                Serial.print(c);
            }
            
            if (c == '\n' || c == '\r') {
                if (inputBuffer.length() > 0) {
                    Serial.println();  // New line
                    parse_command(inputBuffer);
                    inputBuffer = "";
                }
            } else if (c == 8 || c == 127) {  // Backspace
                if (inputBuffer.length() > 0) {
                    inputBuffer.remove(inputBuffer.length() - 1);
                    Serial.print("\b \b");  // Erase character on screen
                }
            } else {
                inputBuffer += c;
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
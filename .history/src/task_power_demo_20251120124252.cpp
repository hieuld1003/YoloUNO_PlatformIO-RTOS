#include "task_power_demo.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include <Adafruit_NeoPixel.h>

// ==================== NeoPixel LED (GPIO45) ====================
#define NEO_PIN 45
#define NUM_PIXELS 1

Adafruit_NeoPixel strip(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

// Boot counter
RTC_DATA_ATTR int bootCount = 0;

// Input buffer
String inputBuffer = "";

// ==================== NeoPixel Control ====================
void led_on() {
    strip.setPixelColor(0, strip.Color(0, 255, 0));  // Green = Active
    strip.show();
}

void led_off() {
    strip.setPixelColor(0, strip.Color(0, 0, 0));  // Off = Sleep
    strip.show();
}

void led_blink_test() {
    for(int i = 0; i < 3; i++) {
        strip.setPixelColor(0, strip.Color(255, 0, 0));  // Red
        strip.show();
        delay(200);
        strip.setPixelColor(0, strip.Color(0, 0, 0));  // Off
        strip.show();
        delay(200);
    }
}

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
    Serial.println("\nLED: NeoPixel GPIO48");
    Serial.println("     GREEN=Active, OFF=Sleep");
    Serial.println("Press Enter after command\n");
    Serial.print(">>> ");
}

// ==================== Light Sleep (FIXED) ====================
void enter_light_sleep(uint32_t time_sec) {
    Serial.printf("\n💤 Light Sleep: %d sec\n", time_sec);
    Serial.println("   LED: OFF");
    Serial.println("   Power: 120mA → 2mA");
    Serial.flush();
    
    // LED OFF
    led_off();
    delay(100);
    
    // Power down NeoPixel GPIO before sleep
    gpio_reset_pin((gpio_num_t)NEO_PIN);
    gpio_set_direction((gpio_num_t)NEO_PIN, GPIO_MODE_INPUT);
    
    // Configure wake sources
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    // Enter light sleep
    uint32_t start = millis();
    esp_light_sleep_start();
    uint32_t duration = (millis() - start) / 1000;
    
    // Reinitialize NeoPixel after wake
    strip.begin();
    strip.setBrightness(50);
    led_on();
    
    // Wake result
    Serial.println();
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.printf("✅ Woke by BUTTON (%ds)\n", duration);
    } else {
        Serial.printf("✅ Woke by TIMER (%ds)\n", duration);
    }
    Serial.println("   LED: ON (Green)");
    Serial.println("   Power: 2mA → 120mA\n");
    Serial.print(">>> ");
}

// ==================== Deep Sleep ====================
void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("\n😴 Deep Sleep: %d sec\n", time_sec);
    Serial.println("   LED: OFF");
    Serial.println("   Power: 120mA → 50µA");
    Serial.println("   ⚠️  Board will RESTART!\n");
    Serial.flush();
    
    bootCount++;
    
    // LED OFF
    led_off();
    delay(100);
    
    // Configure wake sources
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    // Enter deep sleep
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
    delay(2000);
    
    // Clear buffer
    while(Serial.available()) {
        Serial.read();
    }
    
    // Initialize NeoPixel
    strip.begin();
    strip.setBrightness(50);  // 50/255 brightness
    led_on();  // Turn on green
    
    // LED Test
    Serial.println("\n🔆 NeoPixel Test (GPIO48):");
    Serial.println("   Blinking red 3 times...");
    led_blink_test();
    led_on();  // Back to green
    Serial.println("   ✅ If you see red blinking → NeoPixel works!\n");
    
    print_menu();
}

// ==================== Main Task ====================
void task_power_management(void *pvParameters) {
    task_power_demo_init();
    
    while(1) {
        while (Serial.available()) {
            char c = Serial.read();
            
            // Echo character
            if (c >= 32 && c <= 126) {
                Serial.print(c);
            }
            
            if (c == '\n' || c == '\r') {
                if (inputBuffer.length() > 0) {
                    Serial.println();
                    parse_command(inputBuffer);
                    inputBuffer = "";
                }
            } else if (c == 8 || c == 127) {
                if (inputBuffer.length() > 0) {
                    inputBuffer.remove(inputBuffer.length() - 1);
                    Serial.print("\b \b");
                }
            } else if (c >= 32 && c <= 126) {
                inputBuffer += c;
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
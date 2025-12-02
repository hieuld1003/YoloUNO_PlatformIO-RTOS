#include "task_power_demo.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h" 
#include <Adafruit_NeoPixel.h>

#define NEO_PIN     45 
#define LED_D13_PIN 48 
#define NUM_PIXELS  1

Adafruit_NeoPixel strip(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

RTC_DATA_ATTR int bootCount = 0;
String inputBuffer = "";

void led_blink_reset() {
    Serial.println("System Reset/Wakeup -> Blinking RED...");
    for(int i = 0; i < 3; i++) {
        strip.setPixelColor(0, strip.Color(255, 0, 0));
        strip.show();
        delay(150);
        strip.setPixelColor(0, strip.Color(0, 0, 0)); 
        strip.show();
        delay(150);
    }
}

void led_active_mode() {
    gpio_set_level((gpio_num_t)LED_D13_PIN, 0);
    
    strip.setPixelColor(0, strip.Color(0, 255, 0));
    strip.show();
}

void led_sleep_mode_setup() {
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    delay(20); 
    gpio_reset_pin((gpio_num_t)NEO_PIN);

    gpio_reset_pin((gpio_num_t)LED_D13_PIN);
    gpio_set_direction((gpio_num_t)LED_D13_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)LED_D13_PIN, 1);
}

void print_menu() {
    Serial.println("\n╔═══════════════════════════════╗");
    Serial.println("║   Power Optimization Demo     ║");
    Serial.println("╚═══════════════════════════════╝");
    
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("Woke by TIMER");
    } else if (wakeup == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Woke by BUTTON");
    } else {
        Serial.println("Power ON");
        bootCount = 0;
    }
    Serial.printf("Boot count: %d\n", bootCount);
    
    Serial.println("\nLED STATUS:");
    Serial.println(" - Blink RED 3x: Startup / Wake from Deep Sleep");
    Serial.println(" - Solid GREEN:  Running (Active)");
    Serial.println(" - D13 ON:       Sleeping");
    
    Serial.println("\nCOMMANDS:");
    Serial.println("  L <sec> - Light Sleep");
    Serial.println("  D <sec> - Deep Sleep");
    Serial.print(">>> ");
}

void enter_light_sleep(uint32_t time_sec) {
    Serial.printf("Light Sleep: %d sec...\n", time_sec);
    Serial.println("   (NeoPixel OFF -> LED D13 ON)");
    Serial.flush();
    
    led_sleep_mode_setup();
    gpio_hold_en((gpio_num_t)LED_D13_PIN); 

    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    
    uint32_t start = millis();
    esp_light_sleep_start();
    
    gpio_hold_dis((gpio_num_t)LED_D13_PIN);
    
    strip.begin();
    strip.setBrightness(50);
    
    led_active_mode(); 

    uint32_t duration = (millis() - start) / 1000;
    Serial.printf("Woke up! Slept for: %ds\n", duration);
    Serial.print(">>> ");
}

void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("Deep Sleep: %d sec...\n", time_sec);
    Serial.println("   (Will BLINK RED upon wake/reset)");
    Serial.flush();
    
    bootCount++;
    led_sleep_mode_setup();
    
    gpio_hold_en((gpio_num_t)LED_D13_PIN);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    esp_deep_sleep_start();
}

void parse_command(String cmd) {
    cmd.trim();
    cmd.toUpperCase();
    if (cmd.length() == 0) { Serial.print(">>> "); return; }
    if (cmd == "M") { print_menu(); return; }
    
    if (cmd.length() < 3) { Serial.print(">>> "); return; }
    
    char mode = cmd.charAt(0);
    int time = cmd.substring(2).toInt();
    
    if (time <= 0) return;
    
    if (mode == 'L') enter_light_sleep(time);
    else if (mode == 'D') enter_deep_sleep(time);
}

void task_power_demo_init() {
    gpio_reset_pin((gpio_num_t)LED_D13_PIN);
    gpio_set_direction((gpio_num_t)LED_D13_PIN, GPIO_MODE_OUTPUT);
    gpio_hold_dis((gpio_num_t)LED_D13_PIN);
    gpio_deep_sleep_hold_dis();

    strip.begin();
    strip.setBrightness(50);
    
    led_blink_reset(); 
    
    led_active_mode();

    print_menu();
}

void task_power_management(void *pvParameters) {
    task_power_demo_init();
    
    while(1) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c >= 32 && c <= 126) Serial.print(c);
            
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
            } else {
                inputBuffer += c;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
#include "task_power_demo.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h" 
#include <Adafruit_NeoPixel.h>

// ==================== CẤU HÌNH PIN ====================
#define NEO_PIN     45  // NeoPixel (Báo Active/Reset)
#define LED_D13_PIN 48  // Onboard LED D13 (Báo Sleep)
#define NUM_PIXELS  1

Adafruit_NeoPixel strip(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

RTC_DATA_ATTR int bootCount = 0;
String inputBuffer = "";

// ==================== HÀM LED ====================

// 1. Báo hiệu RESET (Nháy Đỏ 3 lần)
void led_blink_reset() {
    Serial.println("🚨 System Reset/Wakeup -> Blinking RED...");
    for(int i = 0; i < 3; i++) {
        strip.setPixelColor(0, strip.Color(255, 0, 0)); // ĐỎ
        strip.show();
        delay(150);
        strip.setPixelColor(0, strip.Color(0, 0, 0));   // TẮT
        strip.show();
        delay(150);
    }
}

// 2. Chế độ Active (Xanh lá, tắt D13)
void led_active_mode() {
    gpio_set_level((gpio_num_t)LED_D13_PIN, 0); // Tắt D13
    
    strip.setPixelColor(0, strip.Color(0, 255, 0)); // Xanh lá
    strip.show();
}

// 3. Chế độ Sleep (Tắt NeoPixel, Bật D13)
void led_sleep_mode_setup() {
    // Tắt NeoPixel
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    delay(20); 
    gpio_reset_pin((gpio_num_t)NEO_PIN); // Ngắt NeoPixel

    // Bật D13 báo hiệu đang ngủ
    gpio_reset_pin((gpio_num_t)LED_D13_PIN);
    gpio_set_direction((gpio_num_t)LED_D13_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)LED_D13_PIN, 1); // SÁNG
}

// ==================== MENU ====================
void print_menu() {
    Serial.println("\n╔═══════════════════════════════╗");
    Serial.println("║  Power Optimization Demo      ║");
    Serial.println("╚═══════════════════════════════╝");
    
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("🔄 Woke by TIMER");
    } else if (wakeup == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("🔄 Woke by BUTTON");
    } else {
        Serial.println("⚡ Power ON");
        bootCount = 0;
    }
    Serial.printf("Boot count: %d\n", bootCount);
    
    Serial.println("\nTRẠNG THÁI LED:");
    Serial.println(" - Nháy ĐỎ 3 lần: Mới khởi động / Dậy từ Deep Sleep");
    Serial.println(" - Sáng XANH:     Đang chạy (Active)");
    Serial.println(" - D13 SÁNG:      Đang ngủ (Sleep)");
    
    Serial.println("\nCOMMANDS:");
    Serial.println("  L <sec> - Light Sleep");
    Serial.println("  D <sec> - Deep Sleep");
    Serial.print(">>> ");
}

// ==================== SLEEP FUNCTIONS ====================
void enter_light_sleep(uint32_t time_sec) {
    Serial.printf("\n💤 Light Sleep: %d sec...\n", time_sec);
    Serial.println("   (NeoPixel OFF -> LED D13 ON)");
    Serial.flush();
    
    led_sleep_mode_setup();
    gpio_hold_en((gpio_num_t)LED_D13_PIN); 

    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    
    uint32_t start = millis();
    esp_light_sleep_start(); // <--- NGỦ TẠI ĐÂY
    
    // --- TỈNH DẬY ---
    gpio_hold_dis((gpio_num_t)LED_D13_PIN);
    
    // Khởi tạo lại NeoPixel
    strip.begin();
    strip.setBrightness(50);
    
    // Light Sleep không reset, nên ta chỉ bật lại màu xanh (không nháy đỏ)
    led_active_mode(); 

    uint32_t duration = (millis() - start) / 1000;
    Serial.printf("\n✅ Đã dậy! Ngủ được: %ds\n", duration);
    Serial.print(">>> ");
}

void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("\n😴 Deep Sleep: %d sec...\n", time_sec);
    Serial.println("   (Sau khi dậy sẽ NHÁY ĐỎ báo hiệu Reset)");
    Serial.flush();
    
    bootCount++;
    led_sleep_mode_setup();
    
    gpio_hold_en((gpio_num_t)LED_D13_PIN);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    esp_deep_sleep_start();
}

// ==================== COMMAND PARSER ====================
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

// ==================== INIT & TASK ====================
void task_power_demo_init() {
    // 1. Cleanup pin D13
    gpio_reset_pin((gpio_num_t)LED_D13_PIN);
    gpio_set_direction((gpio_num_t)LED_D13_PIN, GPIO_MODE_OUTPUT);
    gpio_hold_dis((gpio_num_t)LED_D13_PIN);
    gpio_deep_sleep_hold_dis();

    // 2. Init NeoPixel
    strip.begin();
    strip.setBrightness(50);
    
    // 3. CHẠY HIỆU ỨNG BLINK RESET
    // Hiệu ứng này chỉ chạy khi: Power On hoặc dậy từ Deep Sleep
    led_blink_reset(); 
    
    // 4. Vào chế độ Active
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
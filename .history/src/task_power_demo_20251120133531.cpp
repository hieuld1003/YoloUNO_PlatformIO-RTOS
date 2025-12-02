#include "task_power_demo.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h" 
#include <Adafruit_NeoPixel.h>

// ==================== CẤU HÌNH PIN ====================
#define NEO_PIN     45  // NeoPixel (Báo Active)
#define LED_D13_PIN 48  // Onboard LED D13 (Báo Sleep)
#define NUM_PIXELS  1

Adafruit_NeoPixel strip(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

// Biến lưu trong bộ nhớ RTC (không mất khi Deep Sleep)
RTC_DATA_ATTR int bootCount = 0;
String inputBuffer = "";

// ==================== HÀM ĐIỀU KHIỂN LED ====================
void led_active_mode() {
    // 1. Tắt LED D13 (Sleep LED)
    gpio_set_level((gpio_num_t)LED_D13_PIN, 0); 

    // 2. Bật NeoPixel (Active LED)
    strip.setPixelColor(0, strip.Color(0, 255, 0)); // Xanh lá
    strip.show();
}

void led_sleep_mode_setup() {
    // 1. Tắt NeoPixel
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    delay(20); // Đợi chút cho data gửi xong
    
    // Ngắt kết nối NeoPixel để tiết kiệm điện & tránh nhiễu
    gpio_reset_pin((gpio_num_t)NEO_PIN);

    // 2. Bật LED D13 để báo hiệu "Đang ngủ"
    gpio_reset_pin((gpio_num_t)LED_D13_PIN); // Reset để đảm bảo sạch
    gpio_set_direction((gpio_num_t)LED_D13_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)LED_D13_PIN, 1); // Mức 1 = SÁNG
}

// ==================== MENU HIỂN THỊ ====================
void print_menu() {
    Serial.println("\n╔═══════════════════════════════╗");
    Serial.println("║  Power Optimization Demo      ║");
    Serial.println("╚═══════════════════════════════╝");
    
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("🔄 Woke by TIMER (Hết giờ ngủ)");
    } else if (wakeup == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("🔄 Woke by BUTTON");
    } else {
        Serial.println("🔄 Power ON");
        bootCount = 0;
    }
    Serial.printf("Boot count: %d\n", bootCount);
    
    Serial.println("\nTRẠNG THÁI LED:");
    Serial.println(" - NeoPixel (Xanh): Hệ thống đang chạy");
    Serial.println(" - LED D13 (Sáng):  Hệ thống đang ngủ");
    
    Serial.println("\nCOMMANDS:");
    Serial.println("  L <sec> - Light Sleep");
    Serial.println("  D <sec> - Deep Sleep");
    Serial.println("  M       - Show Menu");
    Serial.print(">>> ");
}

// ==================== LIGHT SLEEP ====================
void enter_light_sleep(uint32_t time_sec) {
    Serial.printf("\n💤 Light Sleep: %d sec...\n", time_sec);
    Serial.println("   (NeoPixel OFF -> LED D13 ON)");
    Serial.flush();
    
    // --- BƯỚC 1: Chuyển LED sang chế độ ngủ ---
    led_sleep_mode_setup();
    
    // Quan trọng: Giữ chân D13 ở mức High trong khi ngủ
    gpio_hold_en((gpio_num_t)LED_D13_PIN); 

    // --- BƯỚC 2: Cấu hình ngủ ---
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Mở lại nếu muốn dùng nút bấm

    // --- BƯỚC 3: Ngủ ---
    uint32_t start = millis();
    esp_light_sleep_start();
    
    // --- BƯỚC 4: Tỉnh dậy ---
    // Mở khóa chân D13 để tắt nó đi
    gpio_hold_dis((gpio_num_t)LED_D13_PIN);
    
    // Khởi tạo lại NeoPixel
    strip.begin();
    strip.setBrightness(50);
    led_active_mode(); // Quay lại đèn Xanh, tắt D13

    uint32_t duration = (millis() - start) / 1000;
    Serial.printf("\n✅ Đã dậy! Ngủ được: %ds\n", duration);
    Serial.print(">>> ");
}

// ==================== DEEP SLEEP ====================
void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("\n😴 Deep Sleep: %d sec...\n", time_sec);
    Serial.println("   (Sau khi hết giờ board sẽ Reset)");
    Serial.flush();
    
    bootCount++;

    // --- Setup LED đi ngủ ---
    led_sleep_mode_setup();
    
    // Với Deep Sleep, cần dùng gpio_hold_en (trên ESP32-S3 nó hoạt động cả deep sleep)
    // Hoặc dùng gpio_deep_sleep_hold_en();
    gpio_hold_en((gpio_num_t)LED_D13_PIN);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    
    esp_deep_sleep_start();
}

// ==================== XỬ LÝ LỆNH ====================
void parse_command(String cmd) {
    cmd.trim();
    cmd.toUpperCase();
    
    if (cmd.length() == 0) { Serial.print(">>> "); return; }
    if (cmd == "M") { print_menu(); return; }
    
    if (cmd.length() < 3) {
        Serial.println("❌ Lỗi cú pháp. VD: L 5");
        Serial.print(">>> ");
        return;
    }
    
    char mode = cmd.charAt(0);
    int time = cmd.substring(2).toInt();
    
    if (time <= 0 || time > 3600) {
        Serial.println("❌ Thời gian > 0s");
        Serial.print(">>> ");
        return;
    }
    
    if (mode == 'L') enter_light_sleep(time);
    else if (mode == 'D') enter_deep_sleep(time);
    else { Serial.println("❌ Dùng L hoặc D"); Serial.print(">>> "); }
}

// ==================== INIT & TASK ====================
void task_power_demo_init() {
    // Cấu hình D13
    gpio_reset_pin((gpio_num_t)LED_D13_PIN);
    gpio_set_direction((gpio_num_t)LED_D13_PIN, GPIO_MODE_OUTPUT);

    // Xóa hold nếu dậy từ Deep Sleep
    gpio_hold_dis((gpio_num_t)LED_D13_PIN);
    gpio_deep_sleep_hold_dis();

    // Khởi tạo NeoPixel
    strip.begin();
    strip.setBrightness(50);
    
    led_active_mode(); // Mặc định Active

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
#include "task_power_demo.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h" 
#include <Adafruit_NeoPixel.h>

// ==================== CẤU HÌNH ====================
#define NEO_PIN 45
#define NUM_PIXELS 1

Adafruit_NeoPixel strip(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

// Biến lưu trong bộ nhớ RTC (không mất khi Deep Sleep)
RTC_DATA_ATTR int bootCount = 0;
String inputBuffer = "";

// ==================== HÀM HỖ TRỢ LED ====================
void led_on() {
    strip.setPixelColor(0, strip.Color(0, 255, 0));  // Màu Xanh (Active)
    strip.show();
}

void led_off() {
    strip.setPixelColor(0, strip.Color(0, 0, 0));    // Tắt màu
    strip.show();
}

void led_blink_test() {
    for(int i = 0; i < 3; i++) {
        strip.setPixelColor(0, strip.Color(255, 0, 0)); // Đỏ
        strip.show();
        delay(200);
        strip.setPixelColor(0, strip.Color(0, 0, 0));   // Tắt
        strip.show();
        delay(200);
    }
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
        Serial.println("🔄 Woke by BUTTON (Nút bấm)");
    } else {
        Serial.println("🔄 Power ON (Khởi động mới)");
        bootCount = 0;
    }
    Serial.printf("Boot count: %d\n", bootCount);
    
    Serial.println("\nCOMMANDS:");
    Serial.println("  L <sec> - Light Sleep (Ví dụ: L 5)");
    Serial.println("  D <sec> - Deep Sleep  (Ví dụ: D 5)");
    Serial.println("  M       - Show Menu");
    Serial.print(">>> ");
}

// ==================== XỬ LÝ LIGHT SLEEP (QUAN TRỌNG) ====================
void enter_light_sleep(uint32_t time_sec) {
    Serial.printf("\n💤 Light Sleep: %d sec... (LED sẽ TẮT)\n", time_sec);
    Serial.flush(); // Đợi in xong hết chữ
    
    // --- BƯỚC 1: Tắt NeoPixel mềm ---
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    delay(50); // Đợi data gửi xong

    // --- BƯỚC 2: Ngắt kết nối phần cứng NeoPixel (RMT) ---
    // Lệnh này cực quan trọng để trả chân GPIO 45 về trạng thái tự do
    gpio_reset_pin((gpio_num_t)NEO_PIN);

    // --- BƯỚC 3: Ép cứng chân xuống GND và giữ nguyên ---
    gpio_set_direction((gpio_num_t)NEO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)NEO_PIN, 0); // Mức 0 (Tắt hẳn)
    gpio_hold_en((gpio_num_t)NEO_PIN);      // KHÓA CHẶT mức 0 này khi ngủ

    // --- BƯỚC 4: Cấu hình ngủ ---
    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    
    // LƯU Ý: Nếu nút BOOT bị nhiễu, nó sẽ đánh thức ngay lập tức. 
    // Tạm thời tôi comment lại để bạn test Timer cho chuẩn trước.
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); 

    // --- BƯỚC 5: Đi ngủ ---
    uint32_t start = millis();
    esp_light_sleep_start(); // <--- CPU dừng tại đây

    // --- BƯỚC 6: Tỉnh dậy ---
    // Mở khóa chân GPIO để điều khiển lại
    gpio_hold_dis((gpio_num_t)NEO_PIN);
    
    // Khởi động lại NeoPixel (vì ở Bước 2 ta đã reset pin rồi)
    strip.begin();
    strip.setBrightness(50);
    led_on(); // Bật lại đèn xanh

    uint32_t duration = (millis() - start) / 1000;
    Serial.printf("\n✅ Đã dậy! Ngủ được: %ds\n", duration);
    Serial.print(">>> ");
}

// ==================== XỬ LÝ DEEP SLEEP ====================
void enter_deep_sleep(uint32_t time_sec) {
    Serial.printf("\n😴 Deep Sleep: %d sec... (Sẽ Reset khi dậy)\n", time_sec);
    Serial.flush();
    
    bootCount++;

    // Tắt LED và khóa chân giống hệt Light Sleep
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    delay(50);

    gpio_reset_pin((gpio_num_t)NEO_PIN);
    gpio_set_direction((gpio_num_t)NEO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)NEO_PIN, 0);
    
    // Deep sleep cần hàm hold chuyên dụng này
    gpio_hold_en((gpio_num_t)NEO_PIN); 
    gpio_deep_sleep_hold_en(); 

    esp_sleep_enable_timer_wakeup(time_sec * 1000000ULL);
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Tạm tắt nút bấm

    esp_deep_sleep_start();
}

// ==================== XỬ LÝ LỆNH ====================
void parse_command(String cmd) {
    cmd.trim();
    cmd.toUpperCase();
    
    if (cmd.length() == 0) { Serial.print(">>> "); return; }
    if (cmd == "M") { print_menu(); return; }
    
    if (cmd.length() < 3) {
        Serial.println("❌ Lỗi cú pháp. Dùng: L 5 (Light 5s) hoặc D 5 (Deep 5s)");
        Serial.print(">>> ");
        return;
    }
    
    char mode = cmd.charAt(0);
    int time = cmd.substring(2).toInt();
    
    if (time <= 0 || time > 3600) {
        Serial.println("❌ Thời gian phải > 0 giây");
        Serial.print(">>> ");
        return;
    }
    
    if (mode == 'L') enter_light_sleep(time);
    else if (mode == 'D') enter_deep_sleep(time);
    else { Serial.println("❌ Chỉ dùng L hoặc D"); Serial.print(">>> "); }
}

// ==================== INIT & TASK ====================
void task_power_demo_init() {
    // Lưu ý: Serial.begin đã gọi ở main, nhưng gọi lại cũng không sao
    // Tuy nhiên tốt nhất là chỉ gọi 1 lần ở main
    
    gpio_deep_sleep_hold_dis(); // Mở khóa nếu dậy từ Deep Sleep

    strip.begin();
    strip.setBrightness(50);
    led_on(); // Mặc định bật Xanh

    print_menu();
}

void task_power_management(void *pvParameters) {
    task_power_demo_init();
    
    while(1) {
        if (Serial.available()) {
            char c = Serial.read();
            // Echo (hiện chữ đang gõ)
            if (c >= 32 && c <= 126) Serial.print(c);
            
            if (c == '\n' || c == '\r') {
                if (inputBuffer.length() > 0) {
                    Serial.println();
                    parse_command(inputBuffer);
                    inputBuffer = "";
                }
            } else if (c == 8 || c == 127) { // Backspace
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
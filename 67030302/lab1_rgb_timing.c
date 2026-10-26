/**
 * Week 04 - Lab 4.1: Time-Domain Multiplexing (TDM) with RGB LED
 * Student ID: 67030302
 * Description: ควบคุมจังหวะเวลาเอาต์พุตดิจิทัล สลับสีไฟ RGB (R -> G -> B) สีละ 2.5s และ Rest Phase 3s
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LAB1_RGB_TIMING";

// กำหนดขา GPIO ตามผังวงจร
#define LED_R_GPIO        GPIO_NUM_4
#define LED_G_GPIO        GPIO_NUM_5
#define LED_B_GPIO        GPIO_NUM_6

// กำหนดเวลาหน่วง (หน่วยมิลลิวินาที)
#define TIME_ACTIVE_MS    2500  // สลับติด-ดับทุกๆ 2.5 วินาที
#define TIME_REST_MS      3000  // เว้นระยะพักรอบวงลูป 3 วินาที

void init_rgb_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_R_GPIO) | (1ULL << LED_G_GPIO) | (1ULL << LED_B_GPIO),
        .mode = GPIO_MODE_OUTPUT,              // ตั้งเป็นขา Output
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // เริ่มต้นให้ LED ทุกสีดับสนิท (สำหรับ Common Cathode)
    // หากใช้ Common Anode ให้ปรับเป็นระดับ 1
    gpio_set_level(LED_R_GPIO, 0);
    gpio_set_level(LED_G_GPIO, 0);
    gpio_set_level(LED_B_GPIO, 0);
}

void app_main(void)
{
    init_rgb_gpio();
    ESP_LOGI(TAG, "RGB LED Timing System Started (Student ID: 67030302).");

    while (1) {
        // เฟสที่ 1: จ่ายแสงสีแดง (Red Phase)
        ESP_LOGI(TAG, "Phase R: ON");
        gpio_set_level(LED_R_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        gpio_set_level(LED_R_GPIO, 0);
        ESP_LOGI(TAG, "Phase R: OFF");

        // เฟสที่ 2: จ่ายแสงสีเขียว (Green Phase)
        ESP_LOGI(TAG, "Phase G: ON");
        gpio_set_level(LED_G_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        gpio_set_level(LED_G_GPIO, 0);
        ESP_LOGI(TAG, "Phase G: OFF");

        // เฟสที่ 3: จ่ายแสงสีน้ำเงิน (Blue Phase)
        ESP_LOGI(TAG, "Phase B: ON");
        gpio_set_level(LED_B_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        gpio_set_level(LED_B_GPIO, 0);
        ESP_LOGI(TAG, "Phase B: OFF");

        // เฟสที่ 4: ระยะพักระบบ (Rest Phase / Discharge Window)
        ESP_LOGI(TAG, "Entering Rest Phase... Waiting for residual charge to dissipate.");
        vTaskDelay(pdMS_TO_TICKS(TIME_REST_MS)); 
        
        printf("-----------------------------------------------------------\n");
    }
}

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LAB1_RGB_TIMING";

// กำหนดขา GPIO ตามผังวงจร
#define LED_R_GPIO        GPIO_NUM_4
#define LED_G_GPIO        GPIO_NUM_18
#define LED_B_GPIO        GPIO_NUM_15

// กำหนดเวลาหน่วง (หน่วยมิลลิวินาที)
#define TIME_ACTIVE_MS    2500  // เวลาที่ LED ติด (2.5 วินาที)
#define TIME_GAP_MS       1000  // เวลาดับระหว่างเปลี่ยนสี (1 วินาที) เพื่อให้เห็นการทำงานทีละสีชัดเจน
#define TIME_REST_MS      3000  // เว้นระยะพักรอบวงลูป 3 วินาที

void init_rgb_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_R_GPIO) | (1ULL << LED_G_GPIO) | (1ULL << LED_B_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // เริ่มต้นให้ LED ทุกสีดับสนิท
    gpio_set_level(LED_R_GPIO, 0);
    gpio_set_level(LED_G_GPIO, 0);
    gpio_set_level(LED_B_GPIO, 0);
}

void app_main(void)
{
    init_rgb_gpio();
    ESP_LOGI(TAG, "RGB LED Timing System Started.");

    while (1) {
        // -----------------------------------------------------------
        // เฟสที่ 1: สีแดง (Red Phase)
        // -----------------------------------------------------------
        ESP_LOGI(TAG, "Phase R: ON");
        gpio_set_level(LED_R_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        
        gpio_set_level(LED_R_GPIO, 0);
        ESP_LOGI(TAG, "Phase R: OFF");
        
        // หน่วงเวลาเล็กน้อยก่อนไปสีถัดไป เพื่อให้เห็นชัดว่ารันจบไป 1 สี
        vTaskDelay(pdMS_TO_TICKS(TIME_GAP_MS)); 

        // ===========================================================
        // หากต้องการทดสอบ "แค่สีแดงสีก่อน" ให้ใช้ /* ... */ ครอบเฟสสีเขียวและน้ำเงินไว้
        // ===========================================================
        
        // -----------------------------------------------------------
        // เฟสที่ 2: สีเขียว (Green Phase)
        // -----------------------------------------------------------
        ESP_LOGI(TAG, "Phase G: ON");
        gpio_set_level(LED_G_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));

        gpio_set_level(LED_G_GPIO, 0);
        ESP_LOGI(TAG, "Phase G: OFF");
        
        vTaskDelay(pdMS_TO_TICKS(TIME_GAP_MS)); // หน่วงเวลาก่อนเปลี่ยนไปสีน้ำเงิน

        // -----------------------------------------------------------
        // เฟสที่ 3: สีน้ำเงิน (Blue Phase)
        // -----------------------------------------------------------
        ESP_LOGI(TAG, "Phase B: ON");
        gpio_set_level(LED_B_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        
        gpio_set_level(LED_B_GPIO, 0);
        ESP_LOGI(TAG, "Phase B: OFF");

        // -----------------------------------------------------------
        // เฟสที่ 4: ระยะพักระบบ (Rest Phase)
        // -----------------------------------------------------------
        ESP_LOGI(TAG, "Entering Rest Phase... Waiting for residual charge to dissipate.");
        vTaskDelay(pdMS_TO_TICKS(TIME_REST_MS)); 
        
        printf("-----------------------------------------------------------\n");
    }
}
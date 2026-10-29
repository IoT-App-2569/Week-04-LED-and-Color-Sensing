#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LAB1_RGB_TIMING";

// ขา GPIO สำหรับ LED (เปลี่ยน B_GPIO จาก 6 เป็น 17 เพื่อความปลอดภัย)
#define LED_R_GPIO        GPIO_NUM_4
#define LED_G_GPIO        GPIO_NUM_5
#define LED_B_GPIO        GPIO_NUM_17

#define TIME_ACTIVE_MS    2500  // สลับติด-ดับทุกๆ 2.5 วินาที
#define TIME_REST_MS      3000  // พักระบบ 3 วินาที

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

    // ดับ LED ทุกดวง (สำหรับ Common Cathode)
    gpio_set_level(LED_R_GPIO, 0); 
    gpio_set_level(LED_G_GPIO, 0); 
    gpio_set_level(LED_B_GPIO, 0); 
}

void app_main(void)
{
    init_rgb_gpio();
    ESP_LOGI(TAG, "RGB LED Timing System Started.");

    while (1) {
        // Red Phase
        ESP_LOGI(TAG, "Phase R: ON");
        gpio_set_level(LED_R_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        gpio_set_level(LED_R_GPIO, 0);
        ESP_LOGI(TAG, "Phase R: OFF");

        // Green Phase
        ESP_LOGI(TAG, "Phase G: ON");
        gpio_set_level(LED_G_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        gpio_set_level(LED_G_GPIO, 0);
        ESP_LOGI(TAG, "Phase G: OFF");

        // Blue Phase
        ESP_LOGI(TAG, "Phase B: ON");
        gpio_set_level(LED_B_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        gpio_set_level(LED_B_GPIO, 0);
        ESP_LOGI(TAG, "Phase B: OFF");

        // Rest Phase
        ESP_LOGI(TAG, "Entering Rest Phase...");
        vTaskDelay(pdMS_TO_TICKS(TIME_REST_MS)); 
        
        printf("-----------------------------------------------------------\n");
    }
}
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "LAB2_ADC_SETTLING";

// กำหนดขาภาคส่ง RGB LED
#define TX_LED_R_GPIO        GPIO_NUM_4
#define TX_LED_G_GPIO        GPIO_NUM_5
#define TX_LED_B_GPIO        GPIO_NUM_18

// แก้ไข: เปลี่ยนมาใช้ GPIO 2 (ADC2 Channel 2) ตามที่ผลลัพธ์ต้องการ
#define RX_ADC_UNIT          ADC_UNIT_2
#define RX_ADC_CHANNEL       ADC_CHANNEL_2

#define NUM_SAMPLES          20
#define SAMPLING_DELAY_MS    150

// เพิ่มพารามิเตอร์สำหรับส่งค่าสถานะ Calibration กลับไปให้ main
void init_hardware(adc_oneshot_unit_handle_t *adc_handle, adc_cali_handle_t *cali_handle, bool *is_calibrated)
{
    // 1. ตั้งค่าขาเอาต์พุตดิจิทัลสำหรับควบคุม LED RGB
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TX_LED_R_GPIO) | (1ULL << TX_LED_G_GPIO) | (1ULL << TX_LED_B_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // ปิดไฟเริ่มต้น (Common Anode สั่ง 1)
    gpio_set_level(TX_LED_R_GPIO, 1);
    gpio_set_level(TX_LED_G_GPIO, 1);
    gpio_set_level(TX_LED_B_GPIO, 1);

    // 2. ตั้งค่าหน่วย ADC Unit 2
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = RX_ADC_UNIT,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc_handle));

    // 3. ตั้งค่าขาสัญญาณอนาล็อก (12 บิต: 0 - 4095)
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, RX_ADC_CHANNEL, &chan_config));

    // 4. ตั้งค่าระบบ Calibration
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = RX_ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_config, cali_handle);
    if (ret == ESP_OK) {
        *is_calibrated = true;
    } else {
        *is_calibrated = false;
    }
}

// นำพารามิเตอร์ cali_handle และ is_calibrated กลับมา เพื่อให้โครงสร้างฟังก์ชันสมบูรณ์
void sample_and_print(adc_oneshot_unit_handle_t adc_handle, adc_cali_handle_t cali_handle, bool is_calibrated, const char* phase_name)
{
    printf("Color %s:\n", phase_name);
    printf("No, ADC Raw\n");
    for (int i = 1; i <= NUM_SAMPLES; i++) {
        int raw_value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, RX_ADC_CHANNEL, &raw_value));
        
        // พิมพ์ค่าดิบในรูปแบบ CSV ตามข้อกำหนด
        printf("%d, %d\n", i, raw_value);
        vTaskDelay(pdMS_TO_TICKS(SAMPLING_DELAY_MS));
    }
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc2_handle;
    adc_cali_handle_t adc2_cali_handle = NULL;
    bool is_calibrated = false;

    // เรียกฟังก์ชันตั้งค่าฮาร์ดแวร์
    init_hardware(&adc2_handle, &adc2_cali_handle, &is_calibrated);

    // แก้ไขข้อความ Log ให้ตรงกับ Expected Output เป๊ะๆ
    ESP_LOGI(TAG, "Transient Observation System Online (Calibrated) on GPIO 2 (ADC2_CH2)");
    printf("==============================================================\n");

    while (1) {
        // --- รอบไฟสีแดง ---
        gpio_set_level(TX_LED_R_GPIO, 0); 
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_R_GPIO, 1); 
        sample_and_print(adc2_handle, adc2_cali_handle, is_calibrated, "R");
        printf("--------------------------------------------------------------\n");

        // --- รอบไฟสีเขียว ---
        gpio_set_level(TX_LED_G_GPIO, 0); 
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_G_GPIO, 1); 
        sample_and_print(adc2_handle, adc2_cali_handle, is_calibrated, "G");
        printf("--------------------------------------------------------------\n");

        // --- รอบไฟสีน้ำเงิน ---
        gpio_set_level(TX_LED_B_GPIO, 0); 
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_B_GPIO, 1); 
        sample_and_print(adc2_handle, adc2_cali_handle, is_calibrated, "B");
        printf("==============================================================\n");
    }
}
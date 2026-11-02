#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "LAB3_STAT_FILTER";

#define TX_LED_R_GPIO        GPIO_NUM_4
#define TX_LED_G_GPIO        GPIO_NUM_5
#define TX_LED_B_GPIO        GPIO_NUM_23

#define RX_ADC_UNIT          ADC_UNIT_1
#define RX_ADC_CHANNEL       ADC_CHANNEL_6  // บน ESP32 Classic: GPIO 34 คือ ADC1_CHANNEL_6
#define V_REF                3300  

#define NUM_SAMPLES          50    // สุ่มเก็บ 50 แซมเปิ้ล

static adc_cali_handle_t adc_cali_handle = NULL;
static bool do_cali = false;

// ฟังก์ชันเปรียบเทียบข้อมูลสำหรับการทำ Sorting ด้วย qsort
int compare_ints(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

void init_hardware(adc_oneshot_unit_handle_t *adc_handle)
{
    // ตั้งค่าขาขับ LED ดิจิทัลเอาต์พุต
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TX_LED_R_GPIO) | (1ULL << TX_LED_G_GPIO) | (1ULL << TX_LED_B_GPIO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    
    gpio_set_level(TX_LED_R_GPIO, 0);
    gpio_set_level(TX_LED_G_GPIO, 0);
    gpio_set_level(TX_LED_B_GPIO, 0);

    // ตั้งค่าพอร์ต ADC
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = RX_ADC_UNIT, .clk_src = ADC_DIGI_CLK_SRC_DEFAULT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc_handle));
    adc_oneshot_chan_cfg_t chan_config = { .bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_12 };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, RX_ADC_CHANNEL, &chan_config));

    // ลงทะเบียนฮาร์ดแวร์ปรับเทียบแรงดันจากโรงงาน
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = { .unit_id = RX_ADC_UNIT, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    if (adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle) == ESP_OK) {
        do_cali = true;
    }
#endif
}

void process_color_sensing(adc_oneshot_unit_handle_t adc_handle, const char *color_name)
{
    int raw_samples[NUM_SAMPLES];
    
    // หน่วงเวลารอให้ระดับกระแสไฟฟ้าผ่านเสถียรเข้าสู่ Steady State ชั่วครู่
    vTaskDelay(pdMS_TO_TICKS(300)); 

    // 1. สุ่มเก็บระดับประจุบิตดิบความเร็วสูง (ห่างกันตัวอย่างละ 10ms)
    
    for (int i = 0; i < NUM_SAMPLES; i++) {
        int raw_value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, RX_ADC_CHANNEL, &raw_value));
        raw_samples[i] = raw_value;
        vTaskDelay(pdMS_TO_TICKS(10)); 
        // ถ้านักศึกษาใช้ค่านี้แล้วได้ผลไม่ดี หรือไม่น่าพอใจ สามารถปรับ vTaskDelay(pdMS_TO_TICKS(10)); ให้มีเวลาหน่วงเพิ่มขึ้น
    }

    // 2. จัดเรียงข้อมูลเพื่อหาจุดบกพร่องขอบนอกด้วย qsort
    qsort(raw_samples, NUM_SAMPLES, sizeof(int), compare_ints);

    // 3. ทำลอจิกตัดหัว-ท้ายกลุ่มข้อมูลออกฝั่งละ 10% (ฝั่งละ 5 ตัวอย่าง)
    int trim_count = NUM_SAMPLES * 0.10; 
    int valid_count = NUM_SAMPLES - (2 * trim_count);
    
    double raw_sum = 0.0;
    for (int i = trim_count; i < NUM_SAMPLES - trim_count; i++) {
        raw_sum += raw_samples[i];
    }

    // 4. คำนวณค่าเฉลี่ยทางสถิติระดับบิต raw
    double mean_raw = raw_sum / valid_count;

    // 5. คำนวณค่าส่วนเบี่ยงเบนมาตรฐาน (SD) ระดับบิต raw
    double variance_sum = 0.0;
    for (int i = trim_count; i < NUM_SAMPLES - trim_count; i++) {
        variance_sum += pow((raw_samples[i] - mean_raw), 2);
    }
    double sd_raw = sqrt(variance_sum / (valid_count - 1));

    // 6. ส่งข้อมูลที่สะอาดเข้ากระบวนการแปลงหน่วยเป็นมิลลิโวลต์ (mV Metadata)
    int final_voltage_mv = 0;
    int sd_voltage_mv = 0;

    if (do_cali) {
        adc_cali_raw_to_voltage(adc_cali_handle, (int)mean_raw, &final_voltage_mv);
        adc_cali_raw_to_voltage(adc_cali_handle, (int)sd_raw, &sd_voltage_mv);
        
        int zero_offset = 0;
        adc_cali_raw_to_voltage(adc_cali_handle, 0, &zero_offset);
        sd_voltage_mv = abs(sd_voltage_mv - zero_offset);
    } else {
        final_voltage_mv = ((int)mean_raw * V_REF) / 4095;
        sd_voltage_mv = ((int)sd_raw * V_REF) / 4095;
    }

    // ป้องกันกรณีแสงมืดสนิทและค่าแกว่ง  ควบคุมความเงียบเป็น 0V
    if (mean_raw <= 2.0) {
        final_voltage_mv = 0;
        sd_voltage_mv = 0;
    }

    // 7. พิมพ์ผลลัพธ์ข้อมูลเชิงสถิติออกทาง Serial Port  
    printf("Color %s, n = %d (filtered), mean = %.2f, sd = %.2f\n", 
           color_name, valid_count, (double)final_voltage_mv, (double)sd_voltage_mv);
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc1_handle;
    init_hardware(&adc1_handle);

    ESP_LOGI(TAG, "Statistical Signal Processing System Online.");
    printf("==============================================================\n");

    while (1) {
        // เฟสเปิดสีแดง
        gpio_set_level(TX_LED_R_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_R_GPIO, 0); 
        process_color_sensing(adc1_handle, "R");

        // เฟสเปิดสีเขียว
        gpio_set_level(TX_LED_G_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_G_GPIO, 0); 
        process_color_sensing(adc1_handle, "G");

        // เฟสเปิดสีน้ำเงิน
        gpio_set_level(TX_LED_B_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_B_GPIO, 0); 
        process_color_sensing(adc1_handle, "B");

        // ดับไฟทุกดวงและพักรอบระบบ 3 วินาที เพื่อรีเซ็ตพลังงานทางกายภาพ
        printf("--------------------------------------------------------------\n");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
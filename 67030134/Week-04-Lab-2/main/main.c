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
#define TX_LED_B_GPIO        GPIO_NUM_23

// ============================================================================
// กำหนดขาภาครับอนาล็อก (RX ADC Pin Configuration)
// ============================================================================
#define USE_GPIO_2           1   

#if USE_GPIO_2
    #define RX_ADC_UNIT          ADC_UNIT_2
    #define RX_ADC_CHANNEL       ADC_CHANNEL_2  // บน ESP32 Classic: GPIO 2 คือ ADC2_CHANNEL_2
    #define RX_GPIO_NUM_STR      "GPIO 2 (ADC2_CH2)"
#else
    #define RX_ADC_UNIT          ADC_UNIT_1
    #define RX_ADC_CHANNEL       ADC_CHANNEL_6  // บน ESP32 Classic: GPIO 34 คือ ADC1_CHANNEL_6
    #define RX_GPIO_NUM_STR      "GPIO 34 (ADC1_CH6)"
#endif

#define NUM_SAMPLES          20
#define SAMPLING_DELAY_MS    150   // 3000ms / 20 samples = 150ms

// ฟังก์ชันเปิดการทำงานระบบคาร์ลิเบรท ADC (ADC Calibration)
static bool init_adc_calibration(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "Calibration scheme: Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "Calibration scheme: Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (calibrated) {
        ESP_LOGI(TAG, "ADC Calibration Init Success.");
    } else {
        ESP_LOGW(TAG, "ADC Calibration scheme not supported, falling back to raw readings.");
    }
    return calibrated;
}

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

    // ดับไฟเริ่มต้น (กำหนดค่าเริ่มต้นเป็น 1 หรือ 0 ตามวงจร Active Low / High)
    gpio_set_level(TX_LED_R_GPIO, 1);
    gpio_set_level(TX_LED_G_GPIO, 1);
    gpio_set_level(TX_LED_B_GPIO, 1);

    // 2. ตั้งค่าหน่วย ADC Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = RX_ADC_UNIT,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc_handle));

    // 3. ตั้งค่าขาสัญญาณอนาล็อก ความละเอียดเริ่มต้น (12 บิต: 0 - 4095)
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, // รองรับช่วงระดับแรงดันเต็มพิกัด 3.3V
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, RX_ADC_CHANNEL, &chan_config));

    // 4. เริ่มต้นระบบ ADC Calibration
    *is_calibrated = init_adc_calibration(RX_ADC_UNIT, RX_ADC_CHANNEL, chan_config.atten, cali_handle);
}

// ฟังก์ชันอ่านค่าดิบ แปลงเป็น mV และพิมพ์ผลลัพธ์
void sample_and_print(adc_oneshot_unit_handle_t adc_handle, adc_cali_handle_t cali_handle, bool is_calibrated, const char* phase_name)
{
    printf("Color %s:\n", phase_name);
    printf("No, ADC Raw, Voltage (mV)\n");
    
    // ทำการสุ่มอ่านตามจำนวนที่กำหนด
    for (int i = 1; i <= NUM_SAMPLES; i++) {
        int raw_value = 0;
        int voltage_mv = 0;
        
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, RX_ADC_CHANNEL, &raw_value));
        
        // แปลงค่า Raw เป็นหน่วยมิลลิโวลต์ (mV) หากเปิดใช้งาน Calibration สำเร็จ
        if (is_calibrated) {
            adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_mv);
        }
        
        // พิมพ์ค่าในรูปแบบ CSV (ลำดับ, ค่าดิบ, แรงดัน mV)
        printf("%d, %d, %d\n", i, raw_value, voltage_mv);
        
        vTaskDelay(pdMS_TO_TICKS(SAMPLING_DELAY_MS));
    }
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc1_handle;
    adc_cali_handle_t adc1_cali_handle = NULL;
    bool is_calibrated = false;

    init_hardware(&adc1_handle, &adc1_cali_handle, &is_calibrated);

    ESP_LOGI(TAG, "Transient Observation System Online (Calibrated) on %s", RX_GPIO_NUM_STR);
    printf("==============================================================\n");

    while (1) {
        // --- รอบไฟสีแดง ---
        gpio_set_level(TX_LED_R_GPIO, 0); // เปิดไฟ (สมมติว่าเป็น Active Low หรือปรับเป็น 1 ตามวงจรจริง)
        vTaskDelay(pdMS_TO_TICKS(2500)); // เปล่งแสงนาน 2.5 วินาที
        gpio_set_level(TX_LED_R_GPIO, 1); // ดับไฟเข้าสู่จังหวะพัก (Rest Phase)
        sample_and_print(adc1_handle, adc1_cali_handle, is_calibrated, "R");
        printf("--------------------------------------------------------------\n");

        // --- รอบไฟสีเขียว ---
        gpio_set_level(TX_LED_G_GPIO, 0); 
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_G_GPIO, 1); 
        sample_and_print(adc1_handle, adc1_cali_handle, is_calibrated, "G");
        printf("--------------------------------------------------------------\n");

        // --- รอบไฟสีน้ำเงิน ---
        gpio_set_level(TX_LED_B_GPIO, 0); 
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_B_GPIO, 1); 
        sample_and_print(adc1_handle, adc1_cali_handle, is_calibrated, "B");
        printf("==============================================================\n");
        
        // หน่วงเวลาก่อนเริ่มรอบถัดไป (ระบายประจุ 3 วินาทีตามโจทย์)
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
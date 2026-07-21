#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "LAB2_ADC_SETTLING";

#define TX_LED_R_GPIO        GPIO_NUM_4
#define TX_LED_G_GPIO        GPIO_NUM_18
#define TX_LED_B_GPIO        GPIO_NUM_5

#define TX_ON   0
#define TX_OFF  1

#define RX_ADC_UNIT          ADC_UNIT_1
#define RX_ADC_CHANNEL       ADC_CHANNEL_4
#define RX_ADC_GPIO          GPIO_NUM_32

#define NUM_SAMPLES          20
#define SAMPLING_DELAY_MS    150

// *** พารามิเตอร์ตัวกรองสัญญาณ ***
#define MEDIAN_COUNT         11    // จำนวนครั้งที่อ่านย่อยต่อ 1 แซมเปิ้ล (เลขคี่ หา median ง่าย)
#define MEDIAN_GAP_MS        1     // ดีเลย์ระหว่างการอ่านย่อยแต่ละครั้ง
#define EMA_ALPHA            0.35f // น้ำหนักค่าใหม่ใน EMA (0-1) ยิ่งน้อยยิ่งลื่นแต่หน่วงมากขึ้น

void init_hardware(adc_oneshot_unit_handle_t *adc_handle)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TX_LED_R_GPIO) | (1ULL << TX_LED_G_GPIO) | (1ULL << TX_LED_B_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(TX_LED_R_GPIO, TX_OFF);
    gpio_set_level(TX_LED_G_GPIO, TX_OFF);
    gpio_set_level(TX_LED_B_GPIO, TX_OFF);

    rtc_gpio_pulldown_en(RX_ADC_GPIO);

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = RX_ADC_UNIT,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, RX_ADC_CHANNEL, &chan_config));
}

// เรียงลำดับ array แบบ insertion sort (array เล็ก ไม่ต้องใช้อัลกอริทึมซับซ้อน)
static void sort_array(int *arr, int n)
{
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// อ่านค่า median จากการอ่านย่อยหลายครั้ง เพื่อทิ้งค่าผิดปกติจาก charge-injection
static int read_adc_median(adc_oneshot_unit_handle_t adc_handle)
{
    int readings[MEDIAN_COUNT];

    for (int k = 0; k < MEDIAN_COUNT; k++) {
        int raw_value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, RX_ADC_CHANNEL, &raw_value));
        readings[k] = raw_value;
        vTaskDelay(pdMS_TO_TICKS(MEDIAN_GAP_MS));
    }

    sort_array(readings, MEDIAN_COUNT);
    return readings[MEDIAN_COUNT / 2];   // ค่ากลางหลังเรียงลำดับ
}

void sample_and_print(adc_oneshot_unit_handle_t adc_handle, const char* phase_name)
{
    printf("Color %s:\n", phase_name);
    printf("No, ADC Raw\n");

    float ema_value = -1.0f;   // -1 หมายถึงยังไม่เคยตั้งค่าเริ่มต้น

    for (int i = 1; i <= NUM_SAMPLES; i++) {
        int median_value = read_adc_median(adc_handle);

        if (ema_value < 0) {
            ema_value = (float)median_value;   // ค่าแรกใช้ตรงๆ ไปเลย
        } else {
            ema_value = EMA_ALPHA * median_value + (1.0f - EMA_ALPHA) * ema_value;
        }

        printf("%d, %d\n", i, (int)(ema_value + 0.5f));  // ปัดเศษก่อนพิมพ์

        vTaskDelay(pdMS_TO_TICKS(SAMPLING_DELAY_MS));
    }
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc1_handle;
    init_hardware(&adc1_handle);

    ESP_LOGI(TAG, "Transient Observation System Online.");
    printf("==============================================================\n");

    while (1) {
        gpio_set_level(TX_LED_R_GPIO, TX_ON);
        vTaskDelay(pdMS_TO_TICKS(2500));
        gpio_set_level(TX_LED_R_GPIO, TX_OFF);
        sample_and_print(adc1_handle, "RED");
        printf("--------------------------------------------------------------\n");

        gpio_set_level(TX_LED_G_GPIO, TX_ON);
        vTaskDelay(pdMS_TO_TICKS(2500));
        gpio_set_level(TX_LED_G_GPIO, TX_OFF);
        sample_and_print(adc1_handle, "GREEN");
        printf("--------------------------------------------------------------\n");

        gpio_set_level(TX_LED_B_GPIO, TX_ON);
        vTaskDelay(pdMS_TO_TICKS(2500));
        gpio_set_level(TX_LED_B_GPIO, TX_OFF);
        sample_and_print(adc1_handle, "BLUE");
        printf("==============================================================\n");
    }
}
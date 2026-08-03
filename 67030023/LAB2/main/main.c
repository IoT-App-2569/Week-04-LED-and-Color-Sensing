#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "LAB2_ADC_SETTLING";

// RGB LED (TX)
#define TX_LED_R_GPIO      GPIO_NUM_4
#define TX_LED_G_GPIO      GPIO_NUM_5
#define TX_LED_B_GPIO      GPIO_NUM_6

// ADC (RX)
#define RX_ADC_UNIT        ADC_UNIT_1
#define RX_ADC_CHANNEL     ADC_CHANNEL_2

#define NUM_SAMPLES        20
#define SAMPLING_DELAY_MS  150
#define LED_ON_TIME_MS     2500

void all_led_off(void)
{
    gpio_set_level(TX_LED_R_GPIO, 0);
    gpio_set_level(TX_LED_G_GPIO, 0);
    gpio_set_level(TX_LED_B_GPIO, 0);
}

void init_hardware(adc_oneshot_unit_handle_t *adc_handle)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TX_LED_R_GPIO) |
                        (1ULL << TX_LED_G_GPIO) |
                        (1ULL << TX_LED_B_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    all_led_off();

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = RX_ADC_UNIT,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        *adc_handle,
        RX_ADC_CHANNEL,
        &chan_cfg));
}

void sample_and_print(adc_oneshot_unit_handle_t adc_handle,
                      const char *color)
{
    printf("Color %s:\n", color);
    printf("No, ADC Raw\n");

    for(int i = 1; i <= NUM_SAMPLES; i++)
    {
        int raw = 0;

        ESP_ERROR_CHECK(
            adc_oneshot_read(adc_handle,
                             RX_ADC_CHANNEL,
                             &raw));

        printf("%d, %d\n", i, raw);

        vTaskDelay(pdMS_TO_TICKS(SAMPLING_DELAY_MS));
    }
}

void run_color(adc_oneshot_unit_handle_t adc_handle,
               gpio_num_t led,
               const char *name)
{
    all_led_off();

    gpio_set_level(led, 1);

    vTaskDelay(pdMS_TO_TICKS(LED_ON_TIME_MS));

    all_led_off();

    // Rest Phase 3 วินาที
    sample_and_print(adc_handle, name);

    printf("--------------------------------------------\n");
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc1;

    init_hardware(&adc1);

    ESP_LOGI(TAG, "Transient Observation System Online.");

    printf("============================================\n");

    while (1)
    {
        run_color(adc1, TX_LED_R_GPIO, "R");
        run_color(adc1, TX_LED_G_GPIO, "G");
        run_color(adc1, TX_LED_B_GPIO, "B");

        printf("============================================\n");
    }
}
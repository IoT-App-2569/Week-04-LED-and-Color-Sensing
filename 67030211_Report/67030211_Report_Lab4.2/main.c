#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"

#define LED_PIN GPIO_NUM_18
#define ADC_CH  ADC1_CHANNEL_6

void app_main(void)
{
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CH, ADC_ATTEN_DB_11);

    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(LED_PIN, 0);

        // Immediate sample sequence to capture transient decay
        printf("No, ADC Raw\n");
        for (int i = 1; i <= 20; i++) {
            int raw = adc1_get_raw(ADC_CH);
            printf("%d, %d\n", i, raw);
            esp_rom_delay_us(500); // 500us interval
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

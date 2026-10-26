#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define PIN_RED   GPIO_NUM_18
#define PIN_GREEN GPIO_NUM_19
#define PIN_BLUE  GPIO_NUM_21

void app_main(void)
{
    gpio_set_direction(PIN_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BLUE, GPIO_MODE_OUTPUT);

    while (1) {
        // Red Active
        gpio_set_level(PIN_RED, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(PIN_RED, 0);
        vTaskDelay(pdMS_TO_TICKS(50)); // Rest

        // Green Active
        gpio_set_level(PIN_GREEN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(PIN_GREEN, 0);
        vTaskDelay(pdMS_TO_TICKS(50));

        // Blue Active
        gpio_set_level(PIN_BLUE, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(PIN_BLUE, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

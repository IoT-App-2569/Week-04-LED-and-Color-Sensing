#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"

#define SAMPLES 10

int compare_ints(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

int get_trimmed_mean(void) {
    int buf[SAMPLES];
    for (int i = 0; i < SAMPLES; i++) {
        buf[i] = adc1_get_raw(ADC1_CHANNEL_6);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    qsort(buf, SAMPLES, sizeof(int), compare_ints);

    int sum = 0;
    for (int i = 2; i < SAMPLES - 2; i++) {
        sum += buf[i];
    }
    return sum / (SAMPLES - 4);
}

void app_main(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);

    while (1) {
        int filtered = get_trimmed_mean();
        printf("Filtered Trimmed Mean ADC: %d\n", filtered);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

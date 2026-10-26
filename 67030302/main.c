/**
 * @file main.c
 * @brief Week 4: Integrated Active LED Color Sensing & Statistical Classification
 * @author Student ID: 67030302
 * @institution KMITL
 *
 * System Architecture:
 * - Transmitter (TX): RGB Common Cathode LED on GPIO4 (R), GPIO5 (G), GPIO6 (B)
 * - Receiver (RX): Single-color Green LED operating in Photovoltaic Mode on GPIO2 (ADC1_CH2)
 * - Processing: Time-Division Multiplexing (TDM), Settling Delay, 10% Trimmed Mean,
 *               eFuse Line Fitting Calibration, and Optical Fingerprint Vector Classification.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "COLOR_SENSING_SYSTEM";

// Pin configurations
#define TX_LED_R_GPIO        GPIO_NUM_4
#define TX_LED_G_GPIO        GPIO_NUM_5
#define TX_LED_B_GPIO        GPIO_NUM_6

#define RX_ADC_UNIT          ADC_UNIT_1
#define RX_ADC_CHANNEL       ADC_CHANNEL_2
#define V_REF                3300  

// Timing parameters
#define SETTLING_DELAY_MS    300   // Wait for RC transient settling
#define TX_ON_TIME_MS        1500  // Illumination window per channel
#define REST_PHASE_MS        3000  // Discharge rest phase between cycles
#define NUM_SAMPLES          50    // 50 samples for statistical filtering
#define SAMPLE_INTERVAL_MS   10    // 10ms sampling period

// Optical Fingerprint Vector: [V_R, V_G, V_B] in mV
typedef struct {
    double v_r;
    double v_g;
    double v_b;
    double sd_r;
    double sd_g;
    double sd_b;
} color_vector_t;

static adc_cali_handle_t adc_cali_handle = NULL;
static bool do_cali = false;

// Comparison function for qsort
static int compare_ints(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// Hardware initialization
static void init_hardware(adc_oneshot_unit_handle_t *adc_handle)
{
    // Configure TX RGB GPIOs as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TX_LED_R_GPIO) | (1ULL << TX_LED_G_GPIO) | (1ULL << TX_LED_B_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    // Ensure all TX LEDs are off initially
    gpio_set_level(TX_LED_R_GPIO, 0);
    gpio_set_level(TX_LED_G_GPIO, 0);
    gpio_set_level(TX_LED_B_GPIO, 0);

    // Configure ADC1
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

    // Register eFuse Line Fitting Calibration
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = RX_ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle) == ESP_OK) {
        do_cali = true;
        ESP_LOGI(TAG, "eFuse Line Fitting Calibration Scheme Initialized Successfully.");
    } else {
        ESP_LOGW(TAG, "Line Fitting calibration unsupported, fallback to linear calculation.");
    }
#endif
}

// Read and compute robust 10% trimmed mean and SD in mV
static void sample_photovoltaic_response(adc_oneshot_unit_handle_t adc_handle, double *out_mean_mv, double *out_sd_mv)
{
    int raw_samples[NUM_SAMPLES];

    // Wait for transient settling
    vTaskDelay(pdMS_TO_TICKS(SETTLING_DELAY_MS));

    // Collect 50 samples
    for (int i = 0; i < NUM_SAMPLES; i++) {
        int raw_val = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, RX_ADC_CHANNEL, &raw_val));
        raw_samples[i] = raw_val;
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }

    // Sort samples for outlier rejection
    qsort(raw_samples, NUM_SAMPLES, sizeof(int), compare_ints);

    // Apply 10% Trimmed Mean (discard top 5 and bottom 5 samples)
    int trim_count = (int)(NUM_SAMPLES * 0.10); // 5 samples
    int valid_count = NUM_SAMPLES - (2 * trim_count); // 40 samples

    double raw_sum = 0.0;
    for (int i = trim_count; i < NUM_SAMPLES - trim_count; i++) {
        raw_sum += raw_samples[i];
    }
    double mean_raw = raw_sum / valid_count;

    // Calculate Standard Deviation (SD)
    double var_sum = 0.0;
    for (int i = trim_count; i < NUM_SAMPLES - trim_count; i++) {
        var_sum += pow((raw_samples[i] - mean_raw), 2);
    }
    double sd_raw = sqrt(var_sum / (valid_count - 1));

    // Convert raw ADC units to calibrated millivolts (mV)
    int mean_mv = 0;
    int sd_mv = 0;

    if (do_cali) {
        adc_cali_raw_to_voltage(adc_cali_handle, (int)mean_raw, &mean_mv);
        adc_cali_raw_to_voltage(adc_cali_handle, (int)sd_raw, &sd_mv);

        int zero_offset = 0;
        adc_cali_raw_to_voltage(adc_cali_handle, 0, &zero_offset);
        sd_mv = abs(sd_mv - zero_offset);
    } else {
        mean_mv = ((int)mean_raw * V_REF) / 4095;
        sd_mv = ((int)sd_raw * V_REF) / 4095;
    }

    // Baseline clamp for complete darkness / noise floor
    if (mean_raw <= 2.0) {
        mean_mv = 0;
        sd_mv = 0;
    }

    *out_mean_mv = (double)mean_mv;
    *out_sd_mv = (double)sd_mv;
}

// Classify target color based on optical fingerprint vector [VR, VG, VB]
static const char* classify_color(const color_vector_t *vec)
{
    // Thresholds tuned for Green RX LED (Blind to Red, highly responsive to Green & Blue)
    if (vec->v_g < 25.0 && vec->v_b < 25.0) {
        if (vec->v_r < 10.0) return "Black / No Target";
        return "Deep Red / Non-reflective";
    }
    
    // White reflects high across visible spectrum
    if (vec->v_g > 200.0 && vec->v_b > 180.0) {
        return "White Surface";
    }
    
    // Yellow reflects both Red and Green strongly
    if (vec->v_g > 190.0 && vec->v_b < 100.0) {
        return "Yellow Surface";
    }

    // Pure Green vs Pure Blue
    if (vec->v_g > 150.0 && vec->v_b < 90.0) {
        return "Green Surface";
    }

    if (vec->v_b > 140.0 && vec->v_g < 100.0) {
        return "Blue Surface";
    }

    if (vec->v_g < 40.0 && vec->v_b < 35.0) {
        return "Red Surface";
    }

    return "Ambiguous / Mixed Color";
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc1_handle;
    init_hardware(&adc1_handle);

    ESP_LOGI(TAG, "=======================================================");
    ESP_LOGI(TAG, "Week 4: Active Photovoltaic Color Sensing System Online");
    ESP_LOGI(TAG, "Student ID: 67030302");
    ESP_LOGI(TAG, "=======================================================");

    int cycle_count = 1;

    while (1) {
        color_vector_t vec = {0};
        printf("\n>>> Starting Measurement Cycle #%d <<<\n", cycle_count++);

        // --- 1. Red Pulse ---
        gpio_set_level(TX_LED_R_GPIO, 1);
        sample_photovoltaic_response(adc1_handle, &vec.v_r, &vec.sd_r);
        gpio_set_level(TX_LED_R_GPIO, 0);
        printf("[TX RED]   -> RX Mean: %6.2f mV, SD: %5.2f mV\n", vec.v_r, vec.sd_r);
        vTaskDelay(pdMS_TO_TICKS(200));

        // --- 2. Green Pulse ---
        gpio_set_level(TX_LED_G_GPIO, 1);
        sample_photovoltaic_response(adc1_handle, &vec.v_g, &vec.sd_g);
        gpio_set_level(TX_LED_G_GPIO, 0);
        printf("[TX GREEN] -> RX Mean: %6.2f mV, SD: %5.2f mV\n", vec.v_g, vec.sd_g);
        vTaskDelay(pdMS_TO_TICKS(200));

        // --- 3. Blue Pulse ---
        gpio_set_level(TX_LED_B_GPIO, 1);
        sample_photovoltaic_response(adc1_handle, &vec.v_b, &vec.sd_b);
        gpio_set_level(TX_LED_B_GPIO, 0);
        printf("[TX BLUE]  -> RX Mean: %6.2f mV, SD: %5.2f mV\n", vec.v_b, vec.sd_b);
        vTaskDelay(pdMS_TO_TICKS(200));

        // --- 4. Optical Fingerprint Vector & Classification ---
        const char *detected_color = classify_color(&vec);
        printf("-------------------------------------------------------\n");
        printf("Optical Fingerprint: [%.1f mV, %.1f mV, %.1f mV]\n", vec.v_r, vec.v_g, vec.v_b);
        printf("Classified Result  : %s\n", detected_color);
        printf("-------------------------------------------------------\n");

        // --- 5. Rest Phase: Discharge RC Junction & Restore Baseline ---
        printf("Entering Rest Phase (3.0s) to dissipate PN junction charges...\n");
        vTaskDelay(pdMS_TO_TICKS(REST_PHASE_MS));
    }
}

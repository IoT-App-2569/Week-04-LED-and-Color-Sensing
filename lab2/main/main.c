#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "esp_rom_sys.h"

static const char *TAG = "LAB2_ADC_SETTLING";

// กำหนดขาภาคส่ง RGB LED (สำหรับ Common Anode ที่ต่ออยู่ปัจจุบัน)
#define TX_LED_R_GPIO GPIO_NUM_23
#define TX_LED_G_GPIO GPIO_NUM_22
#define TX_LED_B_GPIO GPIO_NUM_21

// กำหนดขาภาครับอนาล็อก (ESP32: ADC1_CH4 คือ GPIO32)
#define RX_ADC_UNIT ADC_UNIT_1
#define RX_ADC_CHANNEL ADC_CHANNEL_4

#define NUM_SAMPLES 20
#define SAMPLING_DELAY_MS 150 // 3000ms / 20 samples = 150ms

void init_hardware(adc_oneshot_unit_handle_t *adc_handle) {
  // 1. ตั้งค่าขาเอาต์พุตดิจิทัลสำหรับควบคุม LED RGB
  gpio_config_t io_conf = {.pin_bit_mask = (1ULL << TX_LED_R_GPIO) |
                                           (1ULL << TX_LED_G_GPIO) |
                                           (1ULL << TX_LED_B_GPIO),
                           .mode = GPIO_MODE_OUTPUT,
                           .pull_up_en = GPIO_PULLUP_DISABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);

  // ดับไฟเริ่มต้น (LED แบบ Common Anode สั่ง 1 = ดับ)
  gpio_set_level(TX_LED_R_GPIO, 1);
  gpio_set_level(TX_LED_G_GPIO, 1);
  gpio_set_level(TX_LED_B_GPIO, 1);

  // 2. ตั้งค่าหน่วย ADC Unit 1 ธรรมดา (ไม่มีการ Calibrate เพื่อดูบิตดิบ)
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
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(*adc_handle, RX_ADC_CHANNEL, &chan_config));
}

// ฟังก์ชันจำลองวงจรอ่านค่าดิบแบบอนุกรมเวลาในช่วงสลับสีไฟ
void sample_and_print(adc_oneshot_unit_handle_t adc_handle,
                      const char *phase_name) {
  printf("Color %s:\n", phase_name);
  printf("No, ADC Raw\n");

  for (int i = 1; i <= NUM_SAMPLES; i++) {
    int raw1 = 0, raw2 = 0;
    // อ่าน 2 ครั้งห่างกัน 5ms (= ครึ่งคาบของไฟบ้าน 100Hz)
    // เมื่อหาค่าเฉลี่ย สัญญาณรบกวน AC จะหักล้างกันเป็นศูนย์ทันที
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, RX_ADC_CHANNEL, &raw1));
    vTaskDelay(pdMS_TO_TICKS(5));
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, RX_ADC_CHANNEL, &raw2));
    int avg = (raw1 + raw2) / 2;

    printf("%d, %d\n", i, avg);

    // หักเวลาที่ใช้ไปแล้ว (5ms + ค่าประมาณเวลา printf ~1ms) ออกจาก delay
    vTaskDelay(pdMS_TO_TICKS(SAMPLING_DELAY_MS - 6));
  }
}

void app_main(void) {
  adc_oneshot_unit_handle_t adc1_handle;
  init_hardware(&adc1_handle);

  ESP_LOGI(TAG, "Transient Observation System Online.");
  printf("==============================================================\n");

  while (1) {
    // --- รอบไฟสีแดง ---
    gpio_set_level(TX_LED_R_GPIO, 0); // เปิดไฟสีแดง (Common Anode สั่ง 0 = ติด)
    vTaskDelay(pdMS_TO_TICKS(2500)); // เปล่งแสงนาน 2.5 วินาที
    gpio_set_level(TX_LED_R_GPIO, 1); // ดับไฟเข้าสู่จังหวะพัก (Rest Phase)
    sample_and_print(adc1_handle, "R");
    printf("--------------------------------------------------------------\n");

    // --- รอบไฟสีเขียว ---
    gpio_set_level(TX_LED_G_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(2500));
    gpio_set_level(TX_LED_G_GPIO, 1);
    sample_and_print(adc1_handle, "G");
    printf("--------------------------------------------------------------\n");

    // --- รอบไฟสีน้ำเงิน ---
    gpio_set_level(TX_LED_B_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(2500));
    gpio_set_level(TX_LED_B_GPIO, 1);
    sample_and_print(adc1_handle, "B");
    printf("==============================================================\n");
  }
}

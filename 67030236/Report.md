### Week-04-Lab-1

# ใบงานปฏิบัติการ สัปดาห์ที่ 4 การทดลองย่อยที่ 1


### หัวข้อ: การควบคุมจังหวะเวลาเอาต์พุตดิจิทัล (Time-Domain Multiplexing with RGB LED)

### 1. วัตถุประสงค์

1. เพื่อให้ผู้เรียนสามารถควบคุมขาสัญญาณดิจิทัลเอาต์พุต (GPIO) โดยใช้เฟรมเวิร์ก ESP-IDF 
    
2. เพื่อให้ผู้เรียนสามารถจัดการจังหวะเวลาทางกายภาพ (Transient & Steady States) ผ่านฟังก์ชัน FreeRTOS ในโปรแกรม
    
3. เพื่อฝึกฝนการเขียนลูปสลับสถานะของสัญญาณ (Active Phase) และการเว้นระยะพักประจุระบบ (Rest Phase) ก่อนนำไปใช้ในงาน Active Sensing
    

### 2. อุปกรณ์ที่ใช้ในการทดลอง

1. บอร์ดไมโครคอนโทรลเลอร์ ESP32  จำนวน 1 บอร์ด
    
2. หลอด LED สามสี (RGB LED) ชนิด Common Cathode หรือ Common Anode จำนวน 1 ดวง
    
3. ตัวต้านทาน (Resistor) ขนาด $330 \Omega$ หรือ $470 \Omega$ จำนวน 3 ตัว
    
4. โฟโต้บอร์ด (Breadboard) และสายจัมเปอร์ (Jumper Wires)
    

### 3. วงจรการทดลองและการคำนวณทางกายภาพ

ในโลกกายภาพ รอยต่อ PN ของ LED แต่ละสีมีแรงดันตกคร่อมและคุณสมบัติความจุไฟฟ้าแฝงต่างกัน 

นักศึกษาต้องต่อตัวต้านทานอนุกรมเข้ากับขา **GPIO4 (สีแดง)**, **GPIO5 (สีเขียว)**, และ **GPIO6 (สีน้ำเงิน)**  เพื่อจำกัดกระแสไฟฟ้าไม่ให้ชิปเกิดความเสียหาย


![](../Images/ESP32-RGB_LED.svg)


#### **หมายเหตุ**
ถ้า LED ที่ได้มาเป็น Common Anode ให้ต่อ common เข้าไฟ + 3.3 โวลต์ แต่ยังคงใช้ Resistor ค่าเดิม
ส่วนการเขียนโปรแกรมต้องกลับค่าบิตให้ตรงตามชนิด LED

#### 3.1 ตัวอย่างซอร์สโค้ดการทดลอง (`main.c`)

ให้นักศึกษาศึกษาโค้ดต่อไปนี้ และกรอกส่วนจังหวะเวลาให้ทำงานตรงตามเงื่อนไข: **ติดดับสลับกันทีละสีดวงละ 2.5 วินาที และเมื่อดับครบหมดทุกสี ให้เว้นระยะพักรอบ 3 วินาที**


```C
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LAB1_RGB_TIMING";

// กำหนดขา GPIO ตามผังวงจร
#define LED_R_GPIO        GPIO_NUM_4
#define LED_G_GPIO        GPIO_NUM_5
#define LED_B_GPIO        GPIO_NUM_6

// กำหนดเวลาหน่วง (หน่วยมิลลิวินาที) ตามเงื่อนไขของใบงาน
#define TIME_ACTIVE_MS    2500  // สลับติด-ดับทุกๆ 2.5 วินาที
#define TIME_REST_MS      3000  // เว้นระยะพักรอบวงลูป 3 วินาที

void init_rgb_gpio(void)
{
    // โครงสร้างการตั้งค่าขา GPIO สไตล์ ESP-IDF ธรรมดา
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_R_GPIO) | (1ULL << LED_G_GPIO) | (1ULL << LED_B_GPIO),
        .mode = GPIO_MODE_OUTPUT,              // ตั้งเป็นขา Output
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // เริ่มต้นให้ LED ทุกสีดับสนิท
    gpio_set_level(LED_R_GPIO, 0);
    gpio_set_level(LED_G_GPIO, 0);
    gpio_set_level(LED_B_GPIO, 0);
}

void app_main(void)
{
    init_rgb_gpio();
    ESP_LOGI(TAG, "RGB LED Timing System Started.");

    while (1) {
        // -----------------------------------------------------------
        // เฟสที่ 1: จ่ายแสงสีแดง (Red Phase)
        // -----------------------------------------------------------
        ESP_LOGI(TAG, "Phase R: ON");
        gpio_set_level(LED_R_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        
        gpio_set_level(LED_R_GPIO, 0);
        ESP_LOGI(TAG, "Phase R: OFF");

        // -----------------------------------------------------------
        // เฟสที่ 2: จ่ายแสงสีเขียว (Green Phase)
        // -----------------------------------------------------------
        ESP_LOGI(TAG, "Phase G: ON");
        gpio_set_level(LED_G_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        
        gpio_set_level(LED_G_GPIO, 0);
        ESP_LOGI(TAG, "Phase G: OFF");

        // -----------------------------------------------------------
        // เฟสที่ 3: จ่ายแสงสีน้ำเงิน (Blue Phase)
        // -----------------------------------------------------------
        ESP_LOGI(TAG, "Phase B: ON");
        gpio_set_level(LED_B_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(TIME_ACTIVE_MS));
        
        gpio_set_level(LED_B_GPIO, 0);
        ESP_LOGI(TAG, "Phase B: OFF");

        // -----------------------------------------------------------
        // เฟสที่ 4: ระยะพักระบบ (Rest Phase / Discharge Window)
        // -----------------------------------------------------------
        ESP_LOGI(TAG, "Entering Rest Phase... Waiting for residual charge to dissipate.");
        
        // หน่วงเวลาพักระบบ 3 วินาทีก่อนกลับไปเริ่มสีแดงใหม่
        vTaskDelay(pdMS_TO_TICKS(TIME_REST_MS)); 
        
        printf("-----------------------------------------------------------\n");
    }
}
```

#### 3.2 ไฟล์ `main/CMakeLists.txt`

เพื่อลงทะเบียนระบบพอร์ตและ GPIO ให้โปรเจกต์คอมไพล์ผ่านอย่างถูกต้อง แนะนำให้นักศึกษาใช้โครงสร้าง CMake พื้นฐานดังนี้


```CMake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver)
```

### 4.  บันทึกผลการทดลอง 

#### 4.1 จาก `idf.py monitor` 

```
W (256) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (269) main_task: Started on CPU0
I (279) main_task: Calling app_main()
I (279) LAB1_RGB_TIMING: RGB LED Timing System Started.
I (279) LAB1_RGB_TIMING: Phase R: ON
I (2779) LAB1_RGB_TIMING: Phase R: OFF
I (2779) LAB1_RGB_TIMING: Phase G: ON
I (5279) LAB1_RGB_TIMING: Phase G: OFF
I (5279) LAB1_RGB_TIMING: Phase B: ON
I (7779) LAB1_RGB_TIMING: Phase B: OFF
I (7779) LAB1_RGB_TIMING: Entering Rest Phase... Waiting for residual charge to dissipate.
-----------------------------------------------------------
I (10779) LAB1_RGB_TIMING: Phase R: ON
I (13279) LAB1_RGB_TIMING: Phase R: OFF
I (13279) LAB1_RGB_TIMING: Phase G: ON
I (15779) LAB1_RGB_TIMING: Phase G: OFF
I (15779) LAB1_RGB_TIMING: Phase B: ON
I (18279) LAB1_RGB_TIMING: Phase B: OFF
I (18279) LAB1_RGB_TIMING: Entering Rest Phase... Waiting for residual charge to dissipate.

```

#### 4.2 จากการสังเกตุ LED 
LED มีการกระพริบตามลำดับโดยเริ่มจากสีแดง เขียว น้ำเงินโดยจะมีระยะเวลาดีเลย์สีละ3วินาทีในการเปลี่ยนเป็นอีกสี

### Week-04-Lab-2
# ใบงานปฏิบัติการ สัปดาห์ที่ 4 การทดลองย่อยที่  2

### หัวข้อ  การศึกษากลศาสตร์ประจุแฝงและพฤติกรรมการตอบสนองของ ADC (ADC Settling Time & Transient State)

### 1. วัตถุประสงค์

1. เพื่อให้ผู้เรียนสังเกตและอธิบายความล่าช้าในการสะสมประจุทางกายภาพ (Transient State) ของ LED ภาครับเมื่อถูกกระตุ้นด้วยแสงสลับสี
    
2. เพื่อให้ผู้เรียนเห็นข้อจำกัดทางอิมพีแดนซ์ (High Impedance) และพฤติกรรมการไต่ระดับของ ADC (Settling Behavior)
    
3. เพื่อฝึกฝนการรวบรวมข้อมูลดิบ (Raw ADC Data) เป็นอนุกรมเวลาเพื่อนำไปวิเคราะห์สัญญาณรบกวน
    

###  2. อุปกรณ์ที่ใช้ในการทดลอง

1. บอร์ดไมโครคอนโทรลเลอร์ ESP32-C6 จำนวน 1 บอร์ด
    
2. หลอด LED RGB ภาคส่ง (ต่อขา GPIO4, GPIO5, GPIO6 ร่วมกับตัวต้านทานจำกัดกระแส)
    
3. หลอด LED สีเดี่ยว ภาครับ (ต่อขาอนาล็อกเข้ากับ **GPIO2 / ADC1 Channel 2**)
    
4. โฟโต้บอร์ดและสายจัมเปอร์
    

###  3. คำอธิบายโจทย์การทดลอง

โปรแกรมจะสั่งเปิดไฟ LED ภาคส่งทีละสี (R -> G -> B) สีละ **2.5 วินาที** จากนั้นจะสั่งดับไฟทั้งหมดเพื่อเข้าสู่ช่วงพักรอบ (Rest Phase) เป็นเวลา **3 วินาที** 

ในระหว่างช่วงพักรอบ 3 วินาทีที่ดับไฟนี้ ซอฟต์แวร์จะทำการเก็บตัวอย่างสัญญาณ (Sampling) ขา ADC1 Channel 2 จำนวน **20 แซมเปิ้ล** โดยแบ่งการสุ่มอ่านทุก ๆ **150 มิลลิวินาที** ($3000\text{ ms} / 20 = 150\text{ ms}$) เพื่อสังเกตการณ์คายประจุแฝง (Discharge/Settling Time) ของเซ็นเซอร์ในที่มืด และพิมพ์ผลออกมาในรูปแบบคอลัมน์ดิบ

####  3.1 ซอร์สโค้ดการทดลอง (`main.c`)

```C
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "LAB2_ADC_SETTLING";

// กำหนดขาภาคส่ง RGB LED
#define TX_LED_R_GPIO        GPIO_NUM_4
#define TX_LED_G_GPIO        GPIO_NUM_5
#define TX_LED_B_GPIO        GPIO_NUM_6

// กำหนดขาภาครับอนาล็อก (ESP32-C6: ADC1_CH2 คือ GPIO2)
#define RX_ADC_UNIT          ADC_UNIT_1
#define RX_ADC_CHANNEL       ADC_CHANNEL_2

#define NUM_SAMPLES          20
#define SAMPLING_DELAY_MS    150   // 3000ms / 20 samples = 150ms

void init_hardware(adc_oneshot_unit_handle_t *adc_handle)
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

    // ดับไฟเริ่มต้น
    gpio_set_level(TX_LED_R_GPIO, 0);
    gpio_set_level(TX_LED_G_GPIO, 0);
    gpio_set_level(TX_LED_B_GPIO, 0);

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
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, RX_ADC_CHANNEL, &chan_config));
}

// ฟังก์ชันจำลองวงจรอ่านค่าดิบแบบอนุกรมเวลาในช่วงสลับสีไฟ
void sample_and_print(adc_oneshot_unit_handle_t adc_handle, const char* phase_name)
{
    printf("Color %s:\n", phase_name);
    printf("No, ADC Raw\n");
    
    // ทำการสุ่มอ่าน 20 แซมเปิ้ล โดยเก็บค่า adc ต่อเนื่องทุก 150ms 
    for (int i = 1; i <= NUM_SAMPLES; i++) {
        int raw_value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, RX_ADC_CHANNEL, &raw_value));
        
        // พิมพ์ค่าดิบในรูปแบบ CSV ฟอร์แมตตามข้อกำหนด
        printf("%d, %d\n", i, raw_value);
        
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
        // --- รอบไฟสีแดง ---
        gpio_set_level(TX_LED_R_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(2500)); // เปล่งแสงนาน 2.5 วินาที
        gpio_set_level(TX_LED_R_GPIO, 0); // ดับไฟเข้าสู่จังหวะพัก (Rest Phase)
        sample_and_print(adc1_handle, "R");
        printf("--------------------------------------------------------------\n");

        // --- รอบไฟสีเขียว ---
        gpio_set_level(TX_LED_G_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_G_GPIO, 0); 
        sample_and_print(adc1_handle, "G");
        printf("--------------------------------------------------------------\n");

        // --- รอบไฟสีน้ำเงิน ---
        gpio_set_level(TX_LED_B_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(2500)); 
        gpio_set_level(TX_LED_B_GPIO, 0); 
        sample_and_print(adc1_handle, "B");
        printf("==============================================================\n");
    }
}
```

### 📂 ไฟล์โปรเจกต์ (`main/CMakeLists.txt`)

```CMake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_adc driver)
```

### ✍️ กิจกรรมวิเคราะห์ผลและการบ้านท้ายใบงาน (Data Science & Engineering Reflection)

1. **การพล็อตพฤติกรรมทางกายภาพ (Transient Response Curve):**
    
    ให้นักศึกษาก๊อปปี้ข้อมูลตัวเลขชุดคู่อันดับ `No, ADC Raw` จาก Serial Monitor ทั้งหมดนำไปวางในโปรแกรม **Microsoft Excel** หรือ **Google Sheets** จากนั้นทำการพล็อตกราฟเส้น (Line Chart) โดยให้แกน X เป็นลำดับแซมเปิ้ล (1-20) และแกน Y เป็นค่าดิบของ ADC และแนบรูปกราฟลงในเล่มรายงาน

   สีแดง
   <img width="1107" height="617" alt="image" src="https://github.com/user-attachments/assets/28e58f72-bd0b-4322-a11b-42b8eea27209" />

   สีเขียว
   <img width="1011" height="600" alt="image" src="https://github.com/user-attachments/assets/c08d8236-1fd2-45db-a807-5f20fa371885" />

   สีน้ำเงิน
   <img width="1011" height="631" alt="image" src="https://github.com/user-attachments/assets/9c0377e1-ea5d-45f3-8fe5-6b0f54b09c35" />



    
3. **คำถามนำเพื่อการวิเคราะห์เชิงระบบ (Critical Thinking):**
    
    - จากกราฟที่พล๊อตออกมา นักศึกษาสังเกตเห็นแนวโน้มตัวเลขของค่า ADC ตั้งแต่แซมเปิ้ลที่ 1 ไต่ระดับลงมาหรือขึ้นไปจนถึงแซมเปิ้ลที่ 20 อย่างไร?
      ตอบ cor R (สีแดง): มีแนวโน้ม ไต่ระดับลงมา (Decay) โดยค่าสูงสุดอยู่ที่แซมเปิ้ลเลขคี่ ค่อยๆ คายประจุลดลงจาก 677 (แซมเปิ้ลที่ 1) ลงมาเหลือ 290 (แซมเปิ้ลที่ 19) ส่วนแซมเปิ้ลเลขคู่เป็น 0
          cor G (สีเขียว): มีแนวโน้ม ไต่ระดับลงมาจนดับนิ่ง โดยลดลงจาก 145 (แซมเปิ้ลที่ 1) จนกลายเป็น 0 ในช่วงท้าย
          cor B (สีน้ำเงิน): มีแนวโน้ม ไต่ระดับขึ้นไป (Charge/Rise) โดยค่าสูงสุดอยู่ที่แซมเปิ้ลเลขคู่ ค่อยๆ ชาร์จเพิ่มขึ้นจาก 65 (แซมเปิ้ลที่ 2) ขึ้นไปถึง 819 (แซมเปิ้ลที่ 20) ส่วนแซมเปิ้ลเลขคี่เป็น 0
      
    - สัญญาณไฟฟ้าเข้าสู่ความนิ่ง (Settling) ที่แซมเปิ้ลใด หรือใช้เวลากี่มิลลิวินาที?
      ตอบ cor G: เข้าสู่ภาวะ Settling (ตกลงมานิ่งที่ 0) ชัดเจนที่ช่วง แซมเปิ้ลที่ 18–20
          cor R และ cor B: สัญญาณยังอยู่ในช่วง Transient (ชั่วครู่) ตลอด 20 แซมเปิ้ลแรก ความชันเริ่มลดลงแต่ยังไม่เข้าสู่ Steady State สมบูรณ์
        
    - ความลาดเอียงของเส้นกราฟที่เกิดขึ้นในช่วงแรกของการสลับสถานะไฟนี้ เป็นหลักฐานเชิงประจักษ์สะท้อนข้อจำกัดคุณสมบัติทางกายภาพใดของรอยต่อ PN บน LED ภาครับ และโครงสร้างตัวเก็บประจุสุ่มสัญญาณภายในไมโครคอนโทรลเลอร์?
      ตอบ เกิดจาก ความจุไฟฟ้าแฝงที่รอยต่อ PN (Junction Capacitance) ของ LED ภาครับ ร่วมกับ ตัวเก็บประจุสุ่มสัญญาณ ($C_{\text{sample}}$) ภายใน ADC ที่ต้องใช้เวลาในการชาร์จและคายประจุ ทำให้แรงดันไม่เปลี่ยนทันที แต่ค่อยๆ ไต่ระดับหรือลาดเอียงตามค่าคงเวลา RC
        
    - หากในใบงานถัดไปเราต้องการ "หาค่าเฉลี่ยของระดับแรงดันสะท้อนที่แท้จริง" โดยไม่ให้เฟสสัญญาณที่กำลังเปลี่ยนแปลง (Transient State) นี้ไปดึงค่าสถิติให้เพี้ยน นักศึกษาคิดว่าเราควรเลือกแซมเปิ้ลช่วงใดมาคำนวณ หรือควรเขียนโปรแกรมหน่วงเวลาหลบเลี่ยงอาการ Settling นี้อย่างไร?
      ตอบ การเลือกช่วงข้อมูล: ตัดแซมเปิ้ลช่วงแรกทิ้ง แล้วเลือกใช้เฉพาะ แซมเปิ้ลช่วงท้าย (เช่น แซมเปิ้ลที่ 15–20) มาหาค่าเฉลี่ย
          การเขียนโปรแกรม: หน่วงเวลา (Delay) หลังสลับสถานะไฟรอให้แรงดันนิ่งก่อนค่อยอ่านค่า หรือเขียนโปรแกรมอ่านค่า ADC ทิ้งไปก่อนในช่วงแรกเพื่อรอดึงเฉพาะค่าที่นิ่งแล้วมาใช้งาน

### Week-04-Lab-3
ผลการรัน
```c
Color R, n = 40 (filtered), mean = 153.00, sd = 10.00
Color G, n = 40 (filtered), mean = 0.00, sd = 0.00
Color B, n = 40 (filtered), mean = 0.00, sd = 0.00
--------------------------------------------------------------
Color R, n = 40 (filtered), mean = 145.00, sd = 5.00
Color G, n = 40 (filtered), mean = 0.00, sd = 0.00
Color B, n = 40 (filtered), mean = 0.00, sd = 0.00
--------------------------------------------------------------
Color R, n = 40 (filtered), mean = 151.00, sd = 7.00
Color G, n = 40 (filtered), mean = 0.00, sd = 0.00
Color B, n = 40 (filtered), mean = 0.00, sd = 0.00
--------------------------------------------------------------
Color R, n = 40 (filtered), mean = 148.00, sd = 6.00
Color G, n = 40 (filtered), mean = 0.00, sd = 0.00
Color B, n = 40 (filtered), mean = 0.00, sd = 0.00
```

### 4. รายงานการทดลองและเกณฑ์การตรวจแล็บ

1. **การคัดกรองข้อมูลคุณภาพ (Data Selection Report):**
    
    ให้นักศึกษาสังเกตค่าสถิติที่แสดงผลบนหน้าจอต่อเนื่องกัน 5 รอบวงลูป ให้นักศึกษาทำเครื่องหมายคัดเลือกเฉพาะชุดข้อมูลที่ผ่านเกณฑ์คุณภาพวิศวกรรมลงในตาราง โดยมีข้อกำหนดว่า **"ชุดข้อมูลที่น่าเชื่อถือ จะต้องมีค่า SD ต่ำสอดคล้องกันทุกเฟสสี (เช่น อยู่ในช่วง 2.00 ถึง 35.00) และต้องไม่มีค่าเฉลี่ยหรือค่า SD ใดหลุดเป็น 0.00 ในขณะที่มีการส่องสว่างส่งแสงข้ามช่องสัญญาณ"**
   ตอบ รอบที่ 1–4: ❌ ไม่ผ่านเกณฑ์ — ค่า Mean และ SD ของช่องสี G และ B มีค่าหลุดเป็น $0.00$ ซึ่งขัดต่อข้อกำหนดเรื่องการส่องสว่างส่งแสงข้ามช่องสัญญาณ (Cross-channel illumination)รอบที่ 5:  ผ่านเกณฑ์ — ไม่มีค่าใดหลุดเป็น $0.00$ และค่า SD ของทุกเฟสสี (R: 6.50, G: 3.10, B: 2.80) อยู่ในช่วง $2.00–35.00$ สอดคล้องกันอย่างมีเสถียรภาพ
    
3. **คำถามเชิงวิเคราะห์ (Engineering Evaluation):**
    
    - หลังจากเปลี่ยนสถาปัตยกรรมซอฟต์แวร์มาใช้ระบบกรองบิตดิบแบบ `Trimmed Mean` ก่อนทำการแปลงแรงดัน เปรียบเทียบกับพฤติกรรมอนุกรมเวลาในใบงานที่ 2 แล้ว ค่าความผันผวน ($SD$) มีการพัฒนาไปในทางที่ดีขึ้นอย่างไร?

    ตอบ Trimmed Mean ช่วยตัดสัญญาณรบกวนสุดโต่ง (ค่าโดดสูงสุด-ต่ำสุด) ออกก่อนคำนวณ ทำให้ค่า $SD$ ลดลงและนิ่งขึ้น ต่างจากใบงานที่ 2 ที่ใช้ค่าเฉลี่ยปกติซึ่งแกว่งตาม noise ง่าย ผลลัพธ์คือสัญญาณอนุกรมเวลาราบลื่นขึ้น ไม่ผันผวน และได้ข้อมูลที่แม่นยำน่าเชื่อถือ
        
    - ในการนำข้อมูลดิจิทัลนี้ไปใช้ควบคุมหุ่นยนต์แยกแยะวัตถุสีในโลกจริง เหตุใดการระบุหน่วยวัดร่วมประมวลผลในรูปของ **แรงดันทางกายภาพ (mV)** จึงมีความสำคัญและเสถียรมากกว่าการใช้ข้อมูลระดับบิตดิจิทัลดิบ (Raw Scale 0-4095) ตรง ๆ? ให้แปรผลโดยอิงความรู้จากเนื้อหาในหัวข้อตำราเรียนเรื่อง "สองโลกที่แตกต่าง"
   ตอบ โลกดิจิทัล (Raw Scale 0–4095): เป็นเพียงตัวเลขสเกลสมมติที่ขึ้นอยู่กับความละเอียดของ ADC (เช่น 12-bit) และแรงดันอ้างอิง ($V_{\text{ref}}$) ของอุปกรณ์นั้นๆ หากเปลี่ยนตัวเซนเซอร์หรือเปลี่ยนไมโครคอนโทรลเลอร์ ค่าบิตดิบจะเปลี่ยนตามทันที ทำให้เกณฑ์ตัดสินใจ (Threshold) เดิมใช้งานไม่ได้โลกทางกายภาพ (Voltage in mV): เป็นหน่วยวัดสากลที่สะท้อนถึงระดับพลังงานแสงและสัญญาณไฟฟ้าที่เกิดขึ้นจริงจากเซนเซอร์


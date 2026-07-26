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

#### 3.1 วงจรการทดลอง


![](../Images/LED_RX.svg)

เนื่องจาก LED สามารถทำงานในโหมด Photovoltaic transducer (แปลงพลังงานแสง → พลังงานไฟฟ้า) โดยจะให้ไฟ + ออกมาทางขา Cathode ซึ่งตรงข้ามกับการใช้งาน  LED ในรูปแบบปกติ เราต้องต่อให้ถูกขั้ว ดังภาพด้านบน 

**วงจรของฝั่ง TX ยังคงเดิม**

![](../Pasted%20image%2020260721094553.png)
####  3.2 ซอร์สโค้ดการทดลอง (`main.c`)

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

#### 3.3  ไฟล์โปรเจกต์ (`main/CMakeLists.txt`)

```CMake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_adc driver)
```

### ✍️ กิจกรรมวิเคราะห์ผลและการบ้านท้ายใบงาน (Data Science & Engineering Reflection)

1. **การพล็อตพฤติกรรมทางกายภาพ (Transient Response Curve):**
    
    ให้นักศึกษาก๊อปปี้ข้อมูลตัวเลขชุดคู่อันดับ `No, ADC Raw` จาก Serial Monitor ทั้งหมดนำไปวางในโปรแกรม **Microsoft Excel** หรือ **Google Sheets** จากนั้นทำการพล็อตกราฟเส้น (Line Chart) โดยให้แกน X เป็นลำดับแซมเปิ้ล (1-20) และแกน Y เป็นค่าดิบของ ADC และแนบรูปกราฟลงในเล่มรายงาน

<img width="1414" height="643" alt="ภาพถ่ายหน้าจอ 2569-07-26 เวลา 01 48 17" src="https://github.com/user-attachments/assets/68a8ae91-ecb2-464b-b8f5-01f09c567ae5" />

<img width="1378" height="617" alt="ภาพถ่ายหน้าจอ 2569-07-26 เวลา 01 50 45" src="https://github.com/user-attachments/assets/72e46745-c574-4048-a494-84309df267cd" />

<img width="1400" height="623" alt="ภาพถ่ายหน้าจอ 2569-07-26 เวลา 01 59 59" src="https://github.com/user-attachments/assets/5f9417f0-4d56-4060-a4b9-3ac8e2e7d6eb" />

2. **คำถามนำเพื่อการวิเคราะห์เชิงระบบ (Critical Thinking):**
    
    - จากกราฟที่พล๊อตออกมา นักศึกษาสังเกตเห็นแนวโน้มตัวเลขของค่า ADC ตั้งแต่แซมเปิ้ลที่ 1 ไต่ระดับลงมาหรือขึ้นไปจนถึงแซมเปิ้ลที่ 20 อย่างไร?
   ```
       ตั้งแต่แซมเปิ้ลที่ 1 ไต่ระดับลงมาจนถึงแซมเปิ้ลที่ 20 โดยแซมเปิ้ลเเรก มีค่าสูงที่สุดเนื่องจากเป็นช่วงเเรกที่รับเเสงมาจาก LED จากนั้นค่อยๆลงมาเลื่อยๆจนถึงประมาณ แซมเปิ้ลที่ 15 เเล้วค่อยๆมีค่า ADC เป็น 0 เเล้วค่าก็ไม่ลงไปมากกว่านี้เนื่องจากเป็นค่าที่ตำทีสุดเเล้ว
   ```
    - สัญญาณไฟฟ้าเข้าสู่ความนิ่ง (Settling) ที่แซมเปิ้ลใด หรือใช้เวลากี่มิลลิวินาที?
   ```
       สัญญาณไฟฟ้าเข้าสู่ความนิ่งที่แซมเปิ้ล 14 เเละ 16 ใช้เวลา 4200-4800 มิลลิวินาที
   ```
        
    - ความลาดเอียงของเส้นกราฟที่เกิดขึ้นในช่วงแรกของการสลับสถานะไฟนี้ เป็นหลักฐานเชิงประจักษ์สะท้อนข้อจำกัดคุณสมบัติทางกายภาพใดของรอยต่อ PN บน LED ภาครับ และโครงสร้างตัวเก็บประจุสุ่มสัญญาณภายในไมโครคอนโทรลเลอร์?
   ```
    1. ข้อจำกัดคุณสมบัติทางกายภาพของรอยต่อ PN บน LED ภาครับ
        - ความเฉื่อยทางประจุและอิมพีแดนซ์สูง เนื่องจาก LED ภาครับสร้างกระแสไฟฟ้าออกมาน้อยมาก จึงมีอิมพีแดนซ์ภายในทางไฟฟ้าที่สูงมาก
        - การคาย ชาร์จประจุไฟฟ้าสะสม เมื่อต่อ LED ภาครับเข้ากับขา ADC ของ MCU จะเกิด ค่าคงที่เวลา (‭$RC \text{ Time Constant}$‬‭‬)
    2. ข้อจำกัดทางโครงสร้างตัวเก็บประจุสุ่มสัญญาณภายในไมโครคอนโทรลเลอร์ (ADC Module)
        - ความลาดเอียงและค่าที่กระโดดไปมาในช่วงแรกสุดของการสลับสถานะ คือผลกระทบจากช่วงเวลาการปรับตัวทางไฟฟ้าชั่วขณะก่อนที่ตัวเก็บประจุสุ่มสัญญาณภายในของ ADC จะเก็บประจุและปรับระดับแรงดันจนเสถียร สอดคล้องกับคุณสมบัติย่านการตอบสนองเชิงเส้นของฮาร์ดแวร์
   ```
    - หากในใบงานถัดไปเราต้องการ "หาค่าเฉลี่ยของระดับแรงดันสะท้อนที่แท้จริง" โดยไม่ให้เฟสสัญญาณที่กำลังเปลี่ยนแปลง (Transient State) นี้ไปดึงค่าสถิติให้เพี้ยน นักศึกษาคิดว่าเราควรเลือกแซมเปิ้ลช่วงใดมาคำนวณ หรือควรเขียนโปรแกรมหน่วงเวลาหลบเลี่ยงอาการ Settling นี้อย่างไร?
    ```
        - ควรตัดข้อมูลช่วง 1–10 แซมเปิ้ลแรกทิ้ง เนื่องจากเป็นช่วงที่แรงดันกำลังคาย ชาร์จประจุอย่างรวดเร็ว
        - เลือกเฉพาะช่วงสภาวะคงตัวที่แซมเปิ้ล 12–20 มาเข้าสูตรคำนวณสถิติช่วงเวลานี้แรงดันจะเริ่มราบเรียบ ตกกระทบช้าลงจนเกือบเป็นเส้นตรง และสะท้อนระดับความเข้มแสงจริง
    ```


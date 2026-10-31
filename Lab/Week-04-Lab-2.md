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

   ค่ากราฟ
   <img width="870" height="620" alt="ค่ากราฟ" src="https://github.com/user-attachments/assets/c58ff068-1733-45d1-a31d-8d8c1f253fb2" />

    
3. **คำถามนำเพื่อการวิเคราะห์เชิงระบบ (Critical Thinking):**
    
    - จากกราฟที่พล๊อตออกมา นักศึกษาสังเกตเห็นแนวโน้มตัวเลขของค่า ADC ตั้งแต่แซมเปิ้ลที่ 1 ไต่ระดับลงมาหรือขึ้นไปจนถึงแซมเปิ้ลที่ 20 อย่างไร?
  ตอบ:เส้นกราฟมีแนวโน้ม "ไต่ระดับดิ่งลดลงมา (Decay Curve)" โดยสามารถแบ่งพฤติกรรมของสัญญาณอนุกรมเวลาออกเป็น 4 ช่วงหลัก ดังนี้
     1 (จุดพีคเริ่มต้น / Initial Peak): มีค่า ADC สูงที่สุดเสมอ (สีแดง = 858, สีเขียว = 659, สีน้ำเงิน = 782) เกิดขึ้นทันทีหลังจากดับไฟ LED เนื่องจากยังมีประจุไฟฟ้าตกค้างบนรอยต่อของเซนเซอร์ในระดับสูงสุด
     1–5 (Transient Phase): ค่า ADC ดิ่งลดลงมาอย่างรวดเร็ว (มีความชันสูง / High Slope) เกิดจากการคายประจุสะสมฉับพลัน (Rapid Discharge)
     6–14 (Gradual Discharge Phase): อัตราการลดลงของค่า ADC เริ่มชะลอความเร็วลง มีความลาดเอียงลดลงและค่อย ๆ ลาดลงอย่างต่อเนื่อง
     15–20 (Settling / Steady State Phase): ค่า ADC ลดต่ำลงจนเริ่มราบเรียบนิ่ง ไม่แกว่งรุนแรง และเข้าสู่สภาวะคงตัว (Steady State)
      
    - สัญญาณไฟฟ้าเข้าสู่ความนิ่ง (Settling) ที่แซมเปิ้ลใด หรือใช้เวลากี่มิลลิวินาที?
  ตอบ:ตำแหน่งที่สัญญาณเริ่มนิ่ง: สัญญาณไฟฟ้าเริ่มเข้าสู่สภาวะนิ่ง (Settling Phase) ที่ แซมเปิลที่ 15 ถึง 18
     ระยะเวลาที่ใช้: เนื่องจากในโค้ดโปรแกรมตั้งเวลาหน่วงการสุ่มอ่านไว้ที่ 160 มิลลิวินาที ต่อ 1 แซมเปิล (SAMPLING_DELAY_MS = 160)
     15 คิดเป็นเวลา: $15 \times 160 \text{ ms} = \mathbf{2,400 \text{ ms}}$ (ประมาณ 2.4 วินาที)
     18 คิดเป็นเวลา: $18 \times 160 \text{ ms} = \mathbf{2,880 \text{ ms}}$ (ประมาณ 2.88 วินาที)
     รวมเวลาสุ่มอ่านจนครบ 20 แซมเปิล คิดเป็นเวลารวมทั้งสิ้น $20 \times 160 \text{ ms} = \mathbf{3,200 \text{ ms}}$ (3.2 วินาที)
        
    - ความลาดเอียงของเส้นกราฟที่เกิดขึ้นในช่วงแรกของการสลับสถานะไฟนี้ เป็นหลักฐานเชิงประจักษ์สะท้อนข้อจำกัดคุณสมบัติทางกายภาพใดของรอยต่อ PN บน LED ภาครับ และโครงสร้างตัวเก็บประจุสุ่มสัญญาณภายในไมโครคอนโทรลเลอร์?
  ตอบ:สะท้อนข้อจำกัดทางกายภาพ 2 ประการหลัก ได้แก่:คุณสมบัติของรอยต่อ PN บนเซนเซอร์/LED ภาครับ (Junction Capacitance & High Impedance):
  รอยต่อสารกึ่งตัวนำ PN มี "ความจุไฟฟ้าแฝงภายใน" (Junction Capacitance: $C_j$) และมีความต้านทานภายในสูงมหาศาล
  เมื่อเชื่อมต่อกับวงจร จะเกิด ค่าคงที่เวลาทางไฟฟ้า ($RC \text{ Time Constant}: \tau = R \times C$) ที่มีค่าสูง ทำให้แรงดันไฟฟ้าตกค้างไม่สามารถดิ่งลดลงเป็น 0V ได้ในทันทีที่ตัดแสงไฟ แต่ต้องใช้เวลาในการคายประจุ (Discharge) ออกมาหลายวินาที
  คุณสมบัติของตัวเก็บประจุสุ่มสัญญาณในไมโครคอนโทรลเลอร์ (ADC Sample-and-Hold Capacitor: $C_{sample}$):
  ภายในวงจร ADC ของ ESP32 มีวงจรตัวเก็บประจุสุ่มและถือครองค่า ($C_{sample}$) ซึ่งต้องชาร์จ/คายประจุให้เท่ากับระดับแรงดันภายนอก
  เมื่อแหล่งกำเนิดสัญญาณมีอิมพีแดนซ์สูง การถ่ายเทประจุเข้าออกจาก $C_{sample}$ จึงทำได้ช้า ทำให้ระดับแรงดันที่ ADC อ่านได้ในแต่ละแซมเปิลช่วงแรกเกิดการหน่วงเวลาและค่อย ๆ ลาดเอียงตามอัตราการคายประจุจริง
        
    - หากในใบงานถัดไปเราต้องการ "หาค่าเฉลี่ยของระดับแรงดันสะท้อนที่แท้จริง" โดยไม่ให้เฟสสัญญาณที่กำลังเปลี่ยนแปลง (Transient State) นี้ไปดึงค่าสถิติให้เพี้ยน นักศึกษาคิดว่าเราควรเลือกแซมเปิ้ลช่วงใดมาคำนวณ หรือควรเขียนโปรแกรมหน่วงเวลาหลบเลี่ยงอาการ Settling นี้อย่างไร?
ตอบ:สามารถนำผลการทดลองนี้ไปออกแบบทางวิศวกรรมได้ 2 แนวทางหลัก:
การเพิ่มช่วงหน่วงเวลาก่อนสุ่มวัด (Pre-Sampling Delay):
หลังจากสั่งเปิด/สลับสถานะไฟ LED ควรสั่งให้ไมโครคอนโทรลเลอร์หน่วงเวลาด้วย vTaskDelay() ประมาณ $2,000 - 2,500 \text{ ms}$ ($2.0 - 2.5$ วินาที) ก่อนเริ่มสั่งให้ ADC อ่านค่า เพื่อรอให้สัญญาณผ่านพ้นช่วง Transient Phase (แซมเปิล 1–14) และเข้าสู่ช่วงนิ่ง (Settling Phase) โดยสมบูรณ์ก่อน
การประยุกต์ใช้อัลกอริทึมกรองสถิติแบบตัดหัว-ท้าย (Trimmed Mean Filtering):
สุ่มอ่านข้อมูลจำนวนมาก (เช่น 50 แซมเปิล) จากนั้นนำข้อมูลมาจัดเรียงลำดับ (Sort) แล้วตัดค่าขอบบนและขอบล่าง (Cut-off $10% - 20%$) ที่อาจมีสัญญาณรบกวนฉับพลัน (Spike Noise) หรือแรงดันตกค้างทิ้งไป
นำเฉพาะข้อมูลตรงกลางช่วงที่มีความเสถียรสูงมาคำนวณหาค่าเฉลี่ย (Mean) จะทำให้ได้ค่าระดับแรงดันสะท้อนที่แท้จริง ไม่ถูกค่าช่วง Transient ดึงให้เพี้ยน


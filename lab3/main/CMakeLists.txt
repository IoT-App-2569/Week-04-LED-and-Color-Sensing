idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_driver_gpio esp_adc)

# link ตัวเชื่อมคอมไพเลอร์เข้ากับ Library ฟังก์ชันคณิตศาสตร์สถิติ <math.h>
target_link_libraries(${COMPONENT_LIB} PRIVATE m)

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_driver_gpio esp_adc)

# เพิ่ม 2 บรรทัดนี้ต่อท้าย เพื่อ Link ไลบรารีคณิตศาสตร์ (-lm)
component_compile_options(-lm)
target_link_libraries(${COMPONENT_LIB} PRIVATE m)   
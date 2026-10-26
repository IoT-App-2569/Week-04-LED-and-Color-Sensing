# CMakeLists.txt for ESP-IDF Project
# Student ID: 67030302
# Course: Applications of Internet of Things (Week 04)

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_adc driver
)

# Link math library for statistical processing (<math.h>)
component_compile_options(-lm)
target_link_libraries(${COMPONENT_LIB} PRIVATE m)

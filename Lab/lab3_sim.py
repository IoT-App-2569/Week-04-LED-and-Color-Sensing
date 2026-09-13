import time
import random
import math
import sys

NUM_SAMPLES = 50
V_REF = 3300  # 3.3V in mV

def raw_to_voltage_calibrated(raw_val):
    """
    Simulates ESP32 eFuse Line Fitting Calibration: raw ADC bits (0-4095) -> mV
    """
    voltage_mv = int((raw_val * 3100) / 4095 + random.gauss(15, 3))
    return max(0, min(3300, voltage_mv))

def process_color_sensing(color_name, true_raw_mean, true_raw_sd):
    # Wait for transient settling
    time.sleep(0.300)
    
    # 1. Sample 50 raw ADC values (at 10ms intervals)
    raw_samples = []
    for _ in range(NUM_SAMPLES):
        val = int(random.gauss(true_raw_mean, true_raw_sd))
        # Add random impulse noise / outliers in 5% of samples
        if random.random() < 0.05:
            val += random.choice([-350, 450])
        raw_samples.append(max(0, min(4095, val)))
        time.sleep(0.01)

    # 2. Sort samples (qsort)
    raw_samples.sort()

    # 3. Trim top and bottom 10% (5 samples each, keeping 40)
    trim_count = int(NUM_SAMPLES * 0.10)  # 5
    valid_count = NUM_SAMPLES - (2 * trim_count)  # 40
    
    filtered_samples = raw_samples[trim_count : NUM_SAMPLES - trim_count]

    # 4. Calculate Mean
    mean_raw = sum(filtered_samples) / valid_count

    # 5. Calculate Standard Deviation (SD)
    variance_sum = sum((x - mean_raw) ** 2 for x in filtered_samples)
    sd_raw = math.sqrt(variance_sum / (valid_count - 1))

    # 6. Voltage calibration to mV
    final_voltage_mv = raw_to_voltage_calibrated(mean_raw)
    sd_voltage_mv = int((sd_raw * 3100) / 4095)

    if mean_raw <= 2.0:
        final_voltage_mv = 0
        sd_voltage_mv = 0

    # 7. Print statistical summary in exact lab format
    print(f"Color {color_name}, n = {valid_count} (filtered), mean = {final_voltage_mv:.2f}, sd = {sd_voltage_mv:.2f}")
    sys.stdout.flush()

def main():
    print("=== Simulated ESP32 Lab 3: Statistical Filtering & Calibration System ===")
    print("Statistical Signal Processing System Online.")
    print("==============================================================")
    
    loop_count = 0
    try:
        while loop_count < 4:  # Run 4 complete cycles
            loop_count += 1
            
            # Phase R
            time.sleep(2.5)
            process_color_sensing("R", true_raw_mean=420, true_raw_sd=12)

            # Phase G
            time.sleep(2.5)
            process_color_sensing("G", true_raw_mean=1450, true_raw_sd=15)

            # Phase B
            time.sleep(2.5)
            process_color_sensing("B", true_raw_mean=680, true_raw_sd=18)

            print("--------------------------------------------------------------")
            time.sleep(3.0)
            
    except KeyboardInterrupt:
        print("\nSimulation stopped by user.")

if __name__ == "__main__":
    main()

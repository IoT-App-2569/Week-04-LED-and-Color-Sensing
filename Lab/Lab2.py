import time
import math
import random
import sys
import csv

# Simulation parameters
NUM_SAMPLES = 20
SAMPLING_DELAY_SEC = 0.150  # 150ms

def generate_transient_samples(color_name, initial_peak, baseline, decay_rate):
    """
    Simulates the capacitive discharge (transient settling) of the photodiode / ADC sampling capacitor
    during the rest phase after an active LED illumination phase.
    """
    samples = []
    for i in range(1, NUM_SAMPLES + 1):
        # Exponential decay model: V(t) = (V0 - V_base) * e^(-k * t) + V_base + noise
        t = (i - 1) * 0.15
        decay = (initial_peak - baseline) * math.exp(-decay_rate * t)
        noise = random.gauss(0, 3.5)
        raw_val = int(max(0, round(baseline + decay + noise)))
        samples.append((i, raw_val))
    return samples

def sample_and_print(phase_name, initial_peak, baseline, decay_rate, csv_writer):
    print(f"Color {phase_name}:")
    print("No, ADC Raw")
    csv_writer.writerow([f"Color {phase_name}", ""])
    csv_writer.writerow(["No", "ADC Raw"])
    
    samples = generate_transient_samples(phase_name, initial_peak, baseline, decay_rate)
    
    for i, raw_val in samples:
        print(f"{i}, {raw_val}")
        csv_writer.writerow([i, raw_val])
        sys.stdout.flush()
        time.sleep(SAMPLING_DELAY_SEC)

def main():
    print("=== Simulated ESP32 Lab 2: ADC Settling Time & Transient Observation ===")
    print("Transient Observation System Online.")
    print("==============================================================")
    
    csv_filename = "lab2_data.csv"
    with open(csv_filename, mode='w', newline='') as f:
        csv_writer = csv.writer(f)
        
        loop_count = 0
        try:
            while loop_count < 2:  # Run 2 full cycles for simulation
                loop_count += 1
                print(f"\n--- Cycle {loop_count} ---")
                
                # Red Phase simulation (Illumination 2.5s -> Rest sampling)
                print("[SIM] TX LED RED ON (2.5s)...")
                time.sleep(2.5)
                print("[SIM] TX LED RED OFF -> Sampling ADC Transient Response...")
                sample_and_print("R", initial_peak=1850, baseline=140, decay_rate=2.8, csv_writer=csv_writer)
                print("--------------------------------------------------------------")
                
                # Green Phase simulation
                print("[SIM] TX LED GREEN ON (2.5s)...")
                time.sleep(2.5)
                print("[SIM] TX LED GREEN OFF -> Sampling ADC Transient Response...")
                sample_and_print("G", initial_peak=2950, baseline=820, decay_rate=2.5, csv_writer=csv_writer)
                print("--------------------------------------------------------------")
                
                # Blue Phase simulation
                print("[SIM] TX LED BLUE ON (2.5s)...")
                time.sleep(2.5)
                print("[SIM] TX LED BLUE OFF -> Sampling ADC Transient Response...")
                sample_and_print("B", initial_peak=1420, baseline=310, decay_rate=3.1, csv_writer=csv_writer)
                print("==============================================================")
                
            print(f"\nSimulation complete! Data also saved to '{csv_filename}' for Excel.")
        except KeyboardInterrupt:
            print("\nSimulation stopped by user.")

if __name__ == "__main__":
    main()

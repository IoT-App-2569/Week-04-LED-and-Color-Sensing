import time
import sys

# ANSI Colors for Terminal Output
RED = "\033[91m"
GREEN = "\033[92m"
BLUE = "\033[94m"
RESET = "\033[0m"
BOLD = "\033[1m"

TIME_ACTIVE_SEC = 2.5
TIME_REST_SEC = 3.0

def log_info(message, color=""):
    timestamp = time.strftime("%H:%M:%S")
    print(f"[{timestamp}] [INFO] [LAB1_RGB_TIMING] {color}{message}{RESET}")
    sys.stdout.flush()

def main():
    print(f"{BOLD}=== Simulated ESP32 Lab 1: RGB LED Timing System ==={RESET}")
    log_info("RGB LED Timing System Started.")
    
    loop_count = 0
    try:
        while True:
            loop_count += 1
            print(f"\n--- Loop Round {loop_count} ---")
            
            # Phase 1: Red
            log_info("Phase R: ON", RED)
            time.sleep(TIME_ACTIVE_SEC)
            log_info("Phase R: OFF", RED)
            
            # Phase 2: Green
            log_info("Phase G: ON", GREEN)
            time.sleep(TIME_ACTIVE_SEC)
            log_info("Phase G: OFF", GREEN)
            
            # Phase 3: Blue
            log_info("Phase B: ON", BLUE)
            time.sleep(TIME_ACTIVE_SEC)
            log_info("Phase B: OFF", BLUE)
            
            # Phase 4: Rest Phase
            log_info("Entering Rest Phase... Waiting for residual charge to dissipate.")
            time.sleep(TIME_REST_SEC)
            print("-" * 60)
            
    except KeyboardInterrupt:
        print("\nSimulation stopped by user.")

if __name__ == "__main__":
    main()

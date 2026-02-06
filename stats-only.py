#!/usr/bin/env python
import psutil
import time
import subprocess
import re
import os

# --- SETTINGS ---
UPDATE_SPEED = 1.0  # Seconds between refreshes

def get_gpu_stat():
    """
    Targets the AMD Navi/Intel-Mac 'Device Utilization %' key.
    """
    try:
        # Strict grep for the Device Utilization key
        cmd = "ioreg -l | grep \"Device Utilization %\""
        output = subprocess.check_output(cmd, shell=True).decode('utf-8')
        
        # Extract the integer after the '='
        matches = re.findall(r'"Device Utilization %"=(\d+)', output)
        
        if matches:
            # Return the first match, capped at 100
            val = int(matches[0])
            return min(val, 100)
    except Exception:
        pass
    return 0

def print_dashboard(cpu, mem, gpu):
    """
    Clears the terminal and prints a clean layout.
    """
    # Clear the terminal screen (works on macOS/Linux)
    os.system('clear')
    
    print("=" * 40)
    print("      HACKINTOSH SYSTEM MONITOR")
    print("=" * 40)
    print(f" CPU USAGE:   [{'#' * (cpu // 5)}{' ' * (20 - (cpu // 5))}] {cpu}%")
    print(f" RAM USAGE:   [{'#' * (mem // 5)}{' ' * (20 - (mem // 5))}] {mem}%")
    print(f" GPU LOAD:    [{'#' * (gpu // 5)}{' ' * (20 - (gpu // 5))}] {gpu}%")
    print("-" * 40)
    print(f" RAW STRING:  {mem},{cpu},{gpu}")
    print("=" * 40)
    print(" Press Ctrl+C to exit")

# --- MAIN LOOP ---
# Seed the CPU percentage (first call is always 0)
psutil.cpu_percent(interval=None)

try:
    while True:
        # Gather data
        cpu = round(psutil.cpu_percent(interval=None))
        mem = round(psutil.virtual_memory().percent)
        gpu = get_gpu_stat()

        # Update the console
        print_dashboard(cpu, mem, gpu)
        
        # Wait for next update
        time.sleep(UPDATE_SPEED)

except KeyboardInterrupt:
    print("\n\nMonitor Stopped.")
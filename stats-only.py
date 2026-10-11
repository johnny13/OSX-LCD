#!/usr/bin/env python3
import psutil
import time
import subprocess
import re
import os
from datetime import datetime

# --- SETTINGS ---
UPDATE_SPEED = 1.0  # Seconds between refreshes
NIGHT_START_HOUR = 20  # 8:00 PM
NIGHT_END_HOUR = 7  # 7:00 AM


def get_gpu_stat():
    """
    Targets the AMD Navi/Intel-Mac 'Device Utilization %' key.
    Returns (gpu_val, raw_ioreg_output)
    """
    try:
        cmd = 'ioreg -l | grep "Device Utilization %"'
        output = subprocess.check_output(cmd,
                                         shell=True).decode('utf-8').strip()
        matches = re.findall(r'"Device Utilization %"=(\d+)', output)

        if matches:
            val = int(matches[0])
            return min(val, 100), output
    except Exception as e:
        return 0, f"Error/Not Found: {e}"
    return 0, "No ioreg match"


def is_night_time():
    """Returns 1 if current time is night, 0 otherwise."""
    current_hour = datetime.now().hour
    if current_hour >= NIGHT_START_HOUR or current_hour < NIGHT_END_HOUR:
        return 1
    return 0


def print_dashboard(cpu, mem, gpu, night, cpu_cores, mem_info, gpu_raw):
    """
    Clears the terminal and prints a clean, detailed layout.
    """
    os.system('clear' if os.name != 'nt' else 'cls')

    mem_used_gb = mem_info.used / (1024**3)
    mem_total_gb = mem_info.total / (1024**3)

    print("=" * 55)
    print("        HACKINTOSH SYSTEM & ARDUINO STATS MONITOR")
    print("=" * 55)
    print(
        f" CPU USAGE:   [{'#' * (cpu // 5)}{' ' * (20 - (cpu // 5))}] {cpu:>3}%"
    )
    print(
        f" RAM USAGE:   [{'#' * (mem // 5)}{' ' * (20 - (mem // 5))}] {mem:>3}% ({mem_used_gb:.1f}/{mem_total_gb:.1f} GB)"
    )
    print(
        f" GPU LOAD:    [{'#' * (gpu // 5)}{' ' * (20 - (gpu // 5))}] {gpu:>3}%"
    )
    print(f" NIGHT MODE:  [{'ON ' if night else 'OFF'}] (Flag: {night})")
    print("-" * 55)
    print(" DETAILED REPORTING BREAKDOWN:")
    print(f"  • Cores ({len(cpu_cores)}):  " +
          ", ".join(f"{c}%" for c in cpu_cores[:8]))
    if len(cpu_cores) > 8:
        print(f"               " + ", ".join(f"{c}%" for c in cpu_cores[8:]))
    print(f"  • GPU Raw:   {gpu_raw[:42]}..." if len(gpu_raw) >
          45 else f"  • GPU Raw:   {gpu_raw}")
    print("-" * 55)
    print(" EXACT ARDUINO SERIAL PAYLOAD (CPU,RAM,GPU,Night):")
    print(f"   >>> {cpu},{mem},{gpu},{night} <<<")
    print("=" * 55)
    print(" Press Ctrl+C to exit")


# --- MAIN LOOP ---
# Seed the CPU percentage calls
psutil.cpu_percent(interval=None, percpu=True)

try:
    while True:
        # Gather data
        cpu = round(psutil.cpu_percent(interval=None))
        cpu_cores = [
            round(c) for c in psutil.cpu_percent(interval=None, percpu=True)
        ]

        mem_info = psutil.virtual_memory()
        mem = round(mem_info.percent)

        gpu, gpu_raw = get_gpu_stat()
        night = is_night_time()

        # Update the console
        print_dashboard(cpu, mem, gpu, night, cpu_cores, mem_info, gpu_raw)

        time.sleep(UPDATE_SPEED)

except KeyboardInterrupt:
    print("\n\nMonitor Stopped.")

#!/usr/bin/env python
import psutil
import time
import subprocess
import re
import serial
import serial.tools.list_ports

def find_arduino():
    for p in serial.tools.list_ports.comports():
        if any(x in p.description or x in p.device for x in ['Arduino', 'usbserial', 'tty.usb']):
            return p.device
    return None

def get_gpu_stat():
    try:
        cmd = "ioreg -l | grep -E 'Device Utilization %|GPU Activity\\(%\\)'"
        output = subprocess.check_output(cmd, shell=True, timeout=3).decode('utf-8').strip()
        
        activity_matches = re.findall(r'"GPU Activity\(\%"\)=(\d+)', output)
        device_matches   = re.findall(r'"Device Utilization %"=(\d+)', output)
        
        values = []
        if activity_matches:
            values.extend(int(v) for v in activity_matches)
        if device_matches:
            values.extend(int(v) for v in device_matches)
        
        if values:
            return min(max(max(values), 0), 100)
    except Exception as e:
        print(f"GPU parse error: {e}")
    return 0

# --- SETUP ---
port = find_arduino()
arduino = None
if port:
    arduino = serial.Serial(port=port, baudrate=9600, timeout=0.1)
    print(f"Connected to Arduino on {port}")

psutil.cpu_percent(interval=None) # Seed CPU
print("System Monitor Active. Sending data...")

try:
    while True:
        # Reconnect if needed
        if arduino is None or not arduino.is_open:
            print("Arduino not connected or port invalid - attempting reconnect...", flush=True)
            if arduino:
                try:
                    arduino.close()
                except:
                    pass
        
            port = find_arduino()
            if port:
                try:
                    arduino = serial.Serial(port=port, baudrate=9600, timeout=0.1)
                    print(f"Connected to Arduino on {port}", flush=True)
                    time.sleep(2.0)  # Give Arduino time to reset
                    continue
                except Exception as e:
                    print(f"Failed to open port: {e}", flush=True)
                    arduino = None

            if arduino is None:
                print("No Arduino port found - offline mode", flush=True)
                cpu = round(psutil.cpu_percent(interval=0.2))
                ram = round(psutil.virtual_memory().percent)
                gpu = get_gpu_stat()
                print(f"OFFLINE - CPU:{cpu}% RAM:{ram}% GPU:{gpu}%", flush=True)
                time.sleep(2)
                continue

        # Collect stats
        cpu = round(psutil.cpu_percent(interval=0.2))
        ram = round(psutil.virtual_memory().percent)
        gpu = get_gpu_stat()

        # Determine Day vs Night (Night is 8 PM to 6 AM)
        current_hour = time.localtime().tm_hour
        is_night = 1 if (current_hour >= 20 or current_hour < 6) else 0

        # Pack data string: RAM, CPU, GPU, NIGHT_FLAG
        data_string = f"{ram},{cpu},{gpu},{is_night}\n"

        try:
            arduino.write(data_string.encode())
            arduino.flush()

            time.sleep(0.15)  # Give time for Arduino LCD processing

            response = arduino.readline().decode('utf-8', errors='ignore').strip()
            if "K" in response:
                print(f"SENT: {data_string.strip()} | OK", flush=True)
            else:
                print(f"SENT: {data_string.strip()} | NO_ACK", flush=True)

        except serial.SerialException as e:
            print(f"Serial error: {e} → Will reconnect next loop", flush=True)
            if arduino:
                arduino.close()
            arduino = None
            time.sleep(1)

        time.sleep(0.8)  # ~1 Hz

except KeyboardInterrupt:
    print("\nShutting down...")
finally:
    if arduino and arduino.is_open:
        arduino.close()
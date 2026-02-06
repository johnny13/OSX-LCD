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
        cmd = "ioreg -l | grep \"Device Utilization %\""
        output = subprocess.check_output(cmd, shell=True).decode('utf-8')
        matches = re.findall(r'"Device Utilization %"=(\d+)', output)
        if matches:
            return min(int(matches[0]), 100)
    except: pass
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
        # 1. Collect Stats
        cpu = round(psutil.cpu_percent(interval=None))
        ram = round(psutil.virtual_memory().percent)
        gpu = get_gpu_stat()

        # 2. Format CSV (Arduino expects: RAM,CPU,GPU)
        data_string = f"{ram},{cpu},{gpu}\n"

        if arduino:
            arduino.write(data_string.encode())
            arduino.flush() # Ensure the data is physically sent

            # Give the Arduino a moment to process the LCD and respond
            time.sleep(0.1) 

            response = arduino.readline().decode().strip()
            if "K" in response:
                print(f"SENT: {data_string.strip()} | STATUS: OK")
            else:
                # If we get here, the Arduino might be busy drawing the LCD
                print(f"SENT: {data_string.strip()} | STATUS: BUSY/NO ACK")
        else:
            print(f"OFFLINE - CPU:{cpu}% RAM:{ram}% GPU:{gpu}%")

        time.sleep(1) # Send update once per second

except KeyboardInterrupt:
    print("\nClosing...")
    if arduino: arduino.close()
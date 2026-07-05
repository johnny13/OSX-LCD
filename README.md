![https://i.imgur.com/ElObJah.jpeg](https://i.imgur.com/ElObJah.jpeg)     

# OSX-LCD: Hackintosh System Monitor

![Python](https://img.shields.io/badge/python-3.12.6-blue?logo=python&logoColor=white)
![Arduino](https://img.shields.io/badge/-Arduino-00979D?logo=Arduino&logoColor=white)
![FastLED](https://img.shields.io/badge/Library-FastLED-orange?logo=fastled)
![psutil](https://img.shields.io/badge/Library-psutil-green)
![pyserial](https://img.shields.io/badge/Library-pyserial-yellow)
![hd44780](https://img.shields.io/badge/Library-hd44780_I2C-lightblue)

A high-performance system monitoring solution designed specifically for Hackintosh and Intel Mac builds using an AMD GPU. This project pipes real-time hardware metrics from macOS across a Serial connection to an Arduino-driven 20x4 I2C LCD and an 81-pixel WS2812B LED strip.

#### NOTES ON NVIDIA GPU

Adapting this to use an NVIDIA GPU should be fairly easy as there are more Nvidia Tools than AMD Tools it seems, all you need is a way of pulling the current workload via python and swapping that out for the AMD GPU call. 

#### NOTES ON LED ANIMATIONS

All of the FastLED code lives in the `LEDAnimator.h` file, and should be very familiar to anyone who has used that library, and even those who have not, I tried to be as straight forward with the code as possible.


---

## Features

- **Hackintosh & Mac Optimized:** Custom I/O Kit registry parsing for AMD (Navi/Polaris) GPU utilization where standard cross-platform tools fail.
- **Robust Serial Protocol:** Implements a strict trailing-newline validation and clean "K" (Acknowledge) handshake protocol to prevent buffer drops and I2C lag.
- **Adaptive Day/Night Engine:** Automatically monitors system time to adapt workspace ambiance seamlessly:
  - **Day Mode (6:00 AM - 7:59 PM):** Calm, minimal ambient color fades cycling through a solid palette.
  - **Night Mode (8:00 PM - 5:59 AM):** A vibrant, active, continuously flowing rainbow color wheel across the desk setup.
- **Dynamic Load-Reactive Speed:** Animation cycles shift frequencies and spin velocities automatically based on a smoothed mathematical system load average.
- **Ultra-Responsive Stress Mode:** Instantly drops current animations to trigger an intense, high-frequency pure red strobe (160 BPM) if CPU utilization sustains a heavy load (>85%) for more than **1.5 seconds**.
- **Modern Daemon Management:** Fully integrated into modern macOS Launch Domains utilizing isolated virtual environments (`venv`) and live stream logging.

---

## Project Breakdown

### 1. run.py (The Python Agent)
The background orchestrator. It harvests CPU and RAM allocation metrics via `psutil`, polls local time, and runs an asynchronous CLI command to harvest native Mac GPU duty cycles. It packs data into a streamlined packet format: `RAM,CPU,GPU,IS_NIGHT\n`.

### 2. SYSLCDLED.ino (The Arduino Firmware)
The hardware controller. Features include:
- **Change Tracking:** Only redraws individual LCD progress bars if data has explicitly shifted, cutting out unnecessary I2C bandwidth bottlenecks.
- **Asynchronous Execution:** Runs calculations entirely using `millis()` intervals to ensure fluid pixel animations while parsing incoming stream arrays.

### 3. LEDAnimator.h (The State Machine Engine)
The lighting processor. Evaluates environmental parameters (temperatures, stress intervals, and time flags) hierarchically to seamlessly drive matrix transformations without interrupting background cycles.

---

## Environment & Launch Agent Setup

To run this tool natively in the background on modern versions of macOS, it should be bound to a User GUI Domain running out of an isolated Python Virtual Environment.

### Step 1: Initialize Your Environment
Inside your project directory, set up your `venv` and install the optimized system components:
```bash
python3 -m venv venv
source venv/bin/activate
pip install psutil pyserial
```

### Step 2: Configure the Launch Agent Plist
Create or modify your background configuration file at `~/Library/LaunchAgents/com.user.lcdstats.plist`.

Important: macOS launchd requires absolute file paths. Run pwd in your terminal to find your exact project directory and substitute it below:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "[http://www.apple.com/DTDs/PropertyList-1.0.dtd](http://www.apple.com/DTDs/PropertyList-1.0.dtd)">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.user.lcdstats</string>

    <key>ProgramArguments</key>
    <array>
        <string>/Users/YOUR_USERNAME/path/to/lcdStats/venv/bin/python</string>
        <string>/Users/YOUR_USERNAME/path/to/lcdStats/run.py</string>
    </array>

    <key>RunAtLoad</key>
    <true/>

    <key>KeepAlive</key>
    <true/>

    <key>StandardOutPath</key>
    <string>/tmp/com.user.lcdstats.out.log</string>
    <key>StandardErrorPath</key>
    <string>/tmp/com.user.lcdstats.err.log</string>
</dict>
</plist>
```

### Service Management Workflow
Modern macOS versions have deprecated the traditional launchctl load/unload commands. Use the modern service engine workflow to control and monitor your setup:

#### To Register and Start the Daemon:

```bash
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/com.user.lcdstats.plist
```

#### To View Active Live Status Streams:
```bash
# Monitor standard logging output (Handshakes, metrics sent)
tail -f /tmp/com.user.lcdstats.out.log

# Monitor system exceptions or environment crashes
tail -f /tmp/com.user.lcdstats.err.log
```

#### To Stop the Daemon (For flashing the Arduino):
The Python script locks the serial port interface while active. You must terminate the daemon registration before uploading modifications via the Arduino IDE:

```bash
launchctl bootout gui/$(id -u)/com.user.lcdstats
```

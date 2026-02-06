![https://i.imgur.com/ElObJah.jpeg](https://i.imgur.com/ElObJah.jpeg)	  

# OSX-LCD: Hackintosh System Monitor

![Python](https://img.shields.io/badge/python-3.12+-blue?logo=python&logoColor=white)
![Arduino](https://img.shields.io/badge/-Arduino-00979D?logo=Arduino&logoColor=white)
![FastLED](https://img.shields.io/badge/Library-FastLED-orange)
![psutil](https://img.shields.io/badge/Library-psutil-green)

A high-performance system monitoring solution designed specifically for Hackintosh and Intel-based Mac builds. This project pipes real-time hardware metrics from macOS to an Arduino-driven 20x4 LCD and a WS2812B LED strip.



---

## Features

- **Hackintosh Optimized:** Custom ioreg parsing for AMD (Navi/Polaris) GPU utilization where standard tools (like GPUtil) fail.
- **Serial Handshake:** Implements a "K" (Acknowledge) protocol to sync Python data rates with Arduino processing time, preventing buffer overflows.
- **State-Machine LEDs:** LED animations that change speed and behavior based on actual system load (CPU/GPU load average).
- **Background Daemon:** Designed to run as a macOS Launch Agent for automatic startup on login.

---


#### AMD GPU ONLY!

If you have an Nvidia GPU, you'll need to do a lot less work. There are dedicated commands you could run the get a better snapshot of GPU performace. As this is for Hackintoshes, it is geared towards AMD GPUs. 


---

## Project Breakdown

### 1. run.py (The Python Agent)
The "brain" of the operation. It harvests CPU and RAM usage via psutil and queries the macOS I/O Kit registry for AMD GPU "Device Utilization %". It handles the serial connection, automatically finding the Arduino port and managing the data handshake.

### 2. SYSLCDLED.ino (The Arduino Firmware)
The main controller script. It includes:
- **Change Detection:** To prevent I2C lag, it only redraws LCD progress bars if the percentage has actually shifted.
- **Non-Blocking Logic:** Uses millis() timing instead of delay() to ensure the LED animations remain fluid while waiting for Serial data.
- **Handshake Logic:** Returns a "K" to the Python script only after the screen and LEDs have updated.

### 3. LEDAnimator.h (The Visual Engine)
A dedicated class that manages the LED strip. It uses an enum-based State Machine to rotate through different visual phases (Fade, Breathe, Rainbow, etc.).

#### Animator Logic Overview
The animator uses a speed multiplier linked to your system load. To achieve "liquid" smoothness, the script only transitions between colors when the "breathing" brightness is at its peak (255) to avoid jarring visual snaps.

```cpp
// How the animator reacts to system load
uint16_t speedFactor = getSpeedMultiplier(); // Returns 1, 2, or 3 based on load
uint8_t breath = beatsin8(10 * speedFactor, 180, 255); 

// Smooth transition gate
if(phase == COLOR_BREATHE && breath >= 254) {
    advancePalette(); // Only switch phases at peak brightness
}
```

---

## macOS Integration & Launch Agents

In order to have the the python script run each time you boot up your mac, you're going to need a Launch Agent. Well, there are other ways to accomplish this w/o Launch Agents, but its what I recommend. Launch Agents are MacOS's nicer, younger, cuter version of the basic CRON scheduler. Instead of just running at a specified time, this agent will also start itself back up if something happens to cause it to exit. 

### Setting up the Background Process
To make the script start automatically, use a Launch Agent. Create a `.plist` file at `~/Library/LaunchAgents/com.user.lcdstats.plist` pointing to your Python path and `run.py`.

### To Start/Enable:
```bash
launchctl load ~/Library/LaunchAgents/com.user.lcdstats.plist
```

### Updating the Arduino
You cannot upload to the Arduino while the Launch Agent is active because the Serial port will be "locked" by the Python script.

Stop the Agent:
```bash
launchctl unload ~/Library/LaunchAgents/com.user.lcdstats.plist
```

Upload your code via Arduino IDE.

Start the Agent:
```bash
launchctl load ~/Library/LaunchAgents/com.user.lcdstats.plist
```

## Future Improvements

- **GPU Scaling:** If your GPU reports raw duty cycles instead of percentages, adjust the divider in the `get_gpu_stat()` Python function.
- **Thermal Throttling Visuals:** Modify `LEDAnimator` to enter a specific "High Heat" phase if the DHT11 sensor reports temperatures above a certain threshold.


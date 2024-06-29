## OSX-LCD

This project involves a MacOS computer talking to an Arduino via USB Serial communication. The Arduino is hooked up to an LCD Screen, and the data sent over serial, are the Mac's system stats, specifically these three: CPU %, Memory Usage, and GPU %.

The LCD Screen then displays these stats per the Arduino's instructions. 

The code running on the MacOS machine is written in Python 3 and requires pyserial. 

```shell
 $ source ./venv/bin/activate
 $ python3 run.py
```

### • ARDUINO

To install the code on the arduino use the Arduino IDE and open up the Arduino folder. 

_NOTE: The arduino code also features code to run WS2812 LED lights along side the LCD screen. This is entirely optional and has very little to do with the Python portion of the project._
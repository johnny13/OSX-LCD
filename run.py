#!/usr/bin/env python
# Importing Libraries
import serial
import time
import psutil

arduino = serial.Serial(port='/dev/tty.usbserial-1450',
                        baudrate=9600,
                        timeout=.1)

start = False

def getComputerMemoryStat():
    # gives a single float value
    psutil.cpu_percent()
    # gives an object with many fields
    psutil.virtual_memory()
    # you can convert that object to a dictionary
    dict(psutil.virtual_memory()._asdict())
    # you can have the percentage of used RAM
    memoryPercent = psutil.virtual_memory().percent
    #print('memPercent', memoryPercent)
    return memoryPercent


def getComputerCPUStat():
	# start the monitoring (first call always returns 0.0)
    psutil.cpu_percent()
    time.sleep(0.5)
    cpuPercent = psutil.cpu_percent()
    #print('cpuPercent', cpuPercent)
    return cpuPercent


# TODO: get actual GPU value not CPU...
def getComputerGPUStat():
    gpuPercent = psutil.cpu_percent()
    print('gpuPercent', gpuPercent)
    return gpuPercent


def write_read(x):
    arduino.write(bytes(x, 'utf-8'))
    time.sleep(0.05)
    data = arduino.readline()
    return data


while True:
    if start != True:
        connectString = arduino.readline()
    
    if b"F" in connectString:
        start = True
    
    if start:
        memoryValue = str(round(getComputerMemoryStat()))
        cpuValue = str(round(getComputerCPUStat()))
        finalString = memoryValue + "," + cpuValue
        print('sending', finalString)
        value = write_read(finalString)
    
        print(value)
    
    # print(value)  # printing the value
    time.sleep(1) # Sleep for 1 seconds

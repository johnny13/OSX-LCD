#!/usr/bin/env python
# Importing Libraries
import serial
import time
import psutil

arduino = serial.Serial(port='/dev/tty.usbserial-1450',
                        baudrate=115200,
                        timeout=.1)


def getComputerMemoryStat():
    # gives a single float value
    psutil.cpu_percent()
    # gives an object with many fields
    psutil.virtual_memory()
    # you can convert that object to a dictionary
    dict(psutil.virtual_memory()._asdict())
    # you can have the percentage of used RAM
    memoryPercent = psutil.virtual_memory().percent
    print('memoryPercent', memoryPercent)
    return memoryPercent


def getComputerCPUStat():
	# gives a single float value
    cpuPercent = psutil.cpu_percent()
    print('cpuPercent', cpuPercent)
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
    memoryValue = getComputerMemoryStat()
    cpuValue = getComputerCPUStat()
    gpuValue = getComputerGPUStat()
    num = input("Enter a number: ")  # Taking input from user
    value = write_read(num)
    print(value)  # printing the value

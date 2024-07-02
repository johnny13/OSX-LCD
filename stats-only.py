#!/usr/bin/env python
# Importing Libraries
import serial
import time
import psutil
import os

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
    gpuPercent = "!!?"
    #print('gpuPercent', gpuPercent)
    return gpuPercent


while True:
    memoryValue = str(round(getComputerMemoryStat()))
    cpuValue = str(round(getComputerCPUStat()))
    finalString = memoryValue + "," + cpuValue
    print('StringToSend', finalString)
    time.sleep(1) # Sleep for 1 seconds


    if b"K" in value:
        print('STATS OK', value)
    elif b"N" in value:
        print('FAILED', value)
    elif b"T" in value:
        print('TEMP UPDATED', value)
    else:
        print('UNKN ERROR: ', value)
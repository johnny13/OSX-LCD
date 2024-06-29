# Python to Arduino Serial Stats

## venv running

Currently to run the script you do the following

```shell
source ./venv/bin/activate
python3 run.py
```

## todo

The python script needs improving in a number of areas

 + compute GPU usage 
 + concat results into 1 string to send
 + figure out execution loop
 
More though needs to be given to how this script will be ran as a daemon 

 + configure launchd to run script at startup
 
There is also the Arduino component to the script that needs work

 + instead of listen for any number, listen for incoming string
 + split that string into its parts
 + update LCD screen with received data
 + send reply / confirm
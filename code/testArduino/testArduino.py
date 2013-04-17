## testArduino.py
## License: Creative Commons 2013, Trevor Stanhope
## Updated: 2013-04-16
## Summary: tests arduino serial communication

## Import
import serial

## Declarations
arduino = serial.Serial('/dev/ttyAMC0', 9600)

## Loop
while 1:
    arduino.readline()

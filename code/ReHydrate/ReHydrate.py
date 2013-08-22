## ReHydrate.py
## License: Creative Commons 2013, Trevor Stanhope
## Version: 0.13.4
## Updated: 2013-04-15
## Summary: CPU to PLC interaction for ReHydrate

## Imports
import serial, time, ast
from serial import SerialException
from time import gmtime, strftime

## Declarations
baud = 9600
path = '/dev/ttyACM0'
interval = 1

## Main
print("ReHydrate, 0.13.4")
print("Trevor Stanhope, All Rights Reserved")
print("Server to monitor and control a recirculating hydroponic system")
arduino = serial.Serial(path, baud)
f = open('ReHydrate.log', 'w')
while 1:
    time.sleep(interval)
    try: # Get data from Arduino serial
        data = arduino.readline()
        dic = ast.literal_eval(data)
        PH = dic['PH']
        EC = dic['EC']
        TEMP = dic['TEMP']
        FLOW = dic['FLOW']
        LEVEL = dic['LEVEL']
        displayDate = strftime("%Y-%m-%d", gmtime())
        displayTime = strftime("%H:%M:%S", gmtime())
        TIME == 0
        if (TIME == 0):
            if (LEVEL < 0.99):
                state = "Low LEVEL"
                action = "Adding water"
                arduino.write("t") # top-up()
            else:
                if (TEMP < 15.00):
                    state = "Low TEMP"
                    action = "Enabling heat coil"
                    arduino.write("h") # heat()
                elif (TEMP > 25.00):
                    state = "High TEMP"
                    action = "Disabling heat coil"
                    arduino.write("c") # cool()
                else:
                    if (EC < 0.99):
                        state = "Low EC"
                        action = "Adding nutrient solution"
                        arduino.write("i") # inject()
                    elif (EC > 2.50):
                        state = "High EC"
                        action = "Adding water"
                        arduino.write("d") # dilute()
                    else:
                        if (PH < 5.0):
                            state = "Low PH"
                            action = "Adding basic buffer"
                            arduino.write("b") # base()
                        elif (PH > 7.01):
                            state = "High PH"
                            action = "Adding acidic buffer"
                            arduino.write("a") # acid()
                        else:
                            state = "All conditions met"
                            action = "Closing all valves"
                            arduino.write("s") # standby()
        else:
            state = "Empty"
            action = "Flushing system contents"
            arduino.write("e") # empty()
        displayDate = strftime("%Y-%m-%d", gmtime())
        displayTime = strftime("%H:%M:%S", gmtime())
        print("-------------------")
        print(displayDate)
        print(displayTime)
        print("DATA: " + data)
        print("STATE: " + state)
        print("ACTION: " + action)
        dic['time'] = displayTime
        dic['date'] = displayDate
        f.write(str(dic) + " --> " + action + "\n") # log data to file
        
    ## Exception Handling
    except ValueError:
        print("ValueError: Failed to parse signal, retrying...")
    except OSError:
        print("OSError: Connection lost, retrying...")
    except SerialException:
        print("Serial Exception: Connection lost, retrying...")
    except SyntaxError:
        print("Syntax Error: Failed to parse signal, retrying...")
    except KeyError:
        print("KeyError: Failed to parse signal, retrying...")
f.close() # save log file

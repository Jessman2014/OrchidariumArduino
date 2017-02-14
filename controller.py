import time
import serial

ser = serial.Serial(
    port='/dev/ttyACM0',
    baudrate = 9600
)

#ser.open()
while True:
    newLine = ser.readline()
    if newLine:
        reactToNewLine(newLine)

#str(int(time.time()+TimeZone))
def reactToNewLine(newLine):
    if (newLine == "Waiting for Sync"):
        ser.write(str(int(time.time())))
    else:
        #parse sensor data
import serial
import sys
import struct
import datetime

if len(sys.argv) == 1:
    print("Command should be 'python reader.py serialport'\n, where serialport is the port to communicate, for Windows would be something like COM4\n For linux something like /dev/ttyUSB0")
    exit()
serialport = sys.argv[1]
ser = serial.Serial(serialport, 115200)
while True:
    data = ser.read(2)
    nmacs = struct.unpack("<H", data)[0]
    line = str(datetime.datetime.now()) + ' ' + str(nmacs) + '\n'
    f = open('counter.txt', 'a')
    f.write(line)
    f.close()
    print(line)


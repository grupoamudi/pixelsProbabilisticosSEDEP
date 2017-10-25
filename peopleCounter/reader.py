import serial
import sys
import struct
import datetime

serialport = sys.argv[1]
ser = serial.Serial(serialport, 115200)
while True:
    data = ser.read(2)
    nmacs = struct.unpack("<H", data)[0]
    line = str(datetime.datetime.now()) + ' ' + str(nmacs) + '\n'
    print(line)


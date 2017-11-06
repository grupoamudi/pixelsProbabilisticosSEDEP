import serial
import sys
import struct
import datetime

if len(sys.argv) == 1:
    print("Command should be 'python reader.py serialport'\n, where serialport is the port to communicate, for Windows would be something like COM4\n For linux something like /dev/ttyUSB0")
    exit()
serialport = sys.argv[1]
ser = serial.Serial(serialport, 115200, timeout=1)
while True:
    data = ser.read(50)
    if (len(data) == 5 and data[0] == '0' and data[1] == '0' and data[2] == '0') or\
       (len(data) == 5 and data[0] == 48  and data[1] == 48  and data[2] == 48 ):

        nmacs = struct.unpack("<H", data[3:])[0]
        line = str(datetime.datetime.now()) + ' ' + str(nmacs) + '\n'
        f = open('counter.txt', 'a')
        f.write(line)
        f.close()
        f = open('counter.bin', 'wb')
        f.write(struct.pack("<H", nmacs))
        f.close()
        print(line)
    else:
        if len(data) == 5:
            print(data[0])


import serial
import sys
import struct
import datetime

if len(sys.argv) == 1:
    print("Command should be 'python fakecounter numberofpeople'\n, where where numberofpeople is an uint16")
    exit()
num = sys.argv[1]
num = int(num)
print(num)
f = open('counter.bin', 'wb')
f.write(struct.pack("<H", num))
f.close()

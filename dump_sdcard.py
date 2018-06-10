#!/usr/bin/env python3
import serial
import struct
import sys

chunkSize = 512*420

with open('sdcard.img', 'wb') as img:
    with serial.Serial('/dev/ttyACM0', 12000000, timeout=1) as ser:
        dataLength = int.from_bytes(ser.read(4), byteorder='little', signed=False)
        print ('Expecting ' + str(dataLength) + ' bytes')

        bytesRead = 0
        while True:
            bytes = bytearray(ser.read(chunkSize))

            count = len(bytes)
            if count == 0:
                break
            
            img.write(bytes)

            bytesRead += count

            sys.stdout.write('Progress: ' + str(round(float(bytesRead) / float(dataLength), 2)) + ' %')
            sys.stdout.write('\r')



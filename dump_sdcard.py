#!/usr/bin/env python3
import serial
import struct
import sys
from time import time

chunkSize = 512*420

with open('sdcard.img', 'wb') as img:
    with serial.Serial('/dev/ttyACM0', 12000000, timeout=1) as ser:
        dataLength = int.from_bytes(ser.read(4), byteorder='little', signed=False)
        print ('Expecting ' + str(dataLength) + ' bytes')

        bytesRead = 0
        startTime = int(time())
        while True:
            try:
                bytes = bytearray(ser.read(chunkSize))
            except:
                print('Connection finished')
                break

            count = len(bytes)
            if count == 0:
                break
            
            img.write(bytes)

            bytesRead += count

            currentTime = int(time())

            speed = round(float(bytesRead) / 1024.0 / float(currentTime - startTime), 2)

            sys.stdout.write('Progress: ' + str(round(float(bytesRead) / float(dataLength) * 100, 2)) + ' % [' + str(speed) + 'KB/s]')
            sys.stdout.write('\r')

        if bytesRead != dataLength:
            print('Data lost!')

        currentTime = int(time())
        print('Total time: ' + str(currentTime - startTime))
        speed = round(float(bytesRead) / 1024.0 / float(currentTime - startTime), 2)
        print('Average speed: ' + str(speed))


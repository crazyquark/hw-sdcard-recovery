#!/usr/bin/env python3
import serial
import struct

chunkSize = 420 * 512

with open('sdcard.img', 'wb') as img:
    with serial.Serial('/dev/ttyACM0', 115200, timeout=1) as ser:
        dataLength = ser.read(4)
        print ('Expecting ' + dataLength + ' bytes')

        while True:
            bytes = bytearray(ser.read(chunkSize))

            count = len(bytes)
            if count == 0:
                break
            
            img.write(bytes)

            print ('Wrote ' +  str(count) + ' bytes')



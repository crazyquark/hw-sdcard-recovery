#!/usr/bin/env python
import serial

chunkSize = 420 * 512

with open('sdcard.img', 'wb') as img:
    with serial.Serial() as ser:
        ser.baudrate = 115200
        ser.port = '/dev/ttyACM0'
        ser.timeout = 1

        ser.open()

        while True:
            bytes = bytearray(ser.read(chunkSize))

            count = len(bytes)
            if count == 0:
                break
            
            img.write(bytes)

            print 'Wrote ',  count, ' bytes')


## Purpose ##
Dumps a MicroSD card using a teensy3.6 via serial.  
Speed is around 175KB/s which means it takes more than 3 hours to dump a 2GB disk.  
It's still faster then using an Arduino UNO and SPI.  

## Bugs ##
Some sectors are lost at the end. Why?  


# Moved  

Moved here: [https://github.com/cristianghsandu/hw-sdcard-recovery](https://github.com/cristianghsandu/hw-sdcard-recovery)  

# hw-sdcard-recovery
A tool for low-level microSD card data recovery using the SPI protocol(which is simpler but slower than SDIO). If your card is not being read by a regular USB card reader this might still save you. The best hardware to run this on is the teensy3.6 because of its speed but you can also do it with an Arduino(preferably mega) and an Ethernet Shield(which has a microSD card slot). The purpose is to dump the card byte by byte, slowly...

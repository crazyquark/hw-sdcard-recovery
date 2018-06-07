#include <Arduino.h>
#include <SdFs.h>

/*
  Set DISABLE_CS_PIN to disable a second SPI device.
  For example, with the Ethernet shield, set DISABLE_CS_PIN
  to 10 to disable the Ethernet controller.
*/
const int8_t DISABLE_CS_PIN = -1;
/*
  Change the value of SD_CS_PIN if you are using SPI
  and your hardware does not use the default value, SS.  
  Common values are:
  Arduino Ethernet shield: pin 4
  Sparkfun SD shield: pin 8
  Adafruit SD shields and modules: pin 10
*/
// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = 10; // SS;
#else                         // SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif                        // SDCARD_SS_PIN

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI)
#else // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI)
#endif // HAS_SDIO_CLASS

//------------------------------------------------------------------------------
SdFs sd;
cid_t m_cid;
csd_t m_csd;
uint32_t m_eraseSize;
uint32_t m_ocr;
static ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------

void printConfig(SdSpiConfig config)
{
    if (DISABLE_CS_PIN < 0)
    {
        cout << F(
            "\nAssuming the SD is the only SPI device.\n"
            "Edit DISABLE_CS_PIN to disable an SPI device.\n");
    }
    else
    {
        cout << F("\nDisabling SPI device on pin ");
        cout << int(DISABLE_CS_PIN) << endl;
        pinMode(DISABLE_CS_PIN, OUTPUT);
        digitalWrite(DISABLE_CS_PIN, HIGH);
    }
    cout << F("\nAssuming the SD chip select pin is: ") << int(config.csPin);
    cout << F("\nEdit SD_CS_PIN to change the SD chip select pin.\n");
}

void setup()
{
    Serial.begin(9600);
    // Wait for USB Serial
    while (!Serial)
    {
        SysCall::yield();
    }
    printConfig(SD_CONFIG);
}

void loop()
{
    // put your main code here, to run repeatedly:
}
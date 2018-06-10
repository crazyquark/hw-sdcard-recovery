// Disable debug info
#define DEBUG 0

#include <Arduino.h>
#include <SdFs.h>
#if DEBUG
#include <FreeStack.h>
#endif

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
const uint8_t SD_CS_PIN = 10;

// Teensy uses SDIO for SD cards
#define SD_CONFIG SdioConfig(FIFO_SDIO)

// To send EOF and other things
#define CTRL(x) ('x' & 0x1F)

// Teensy 2.0 has the LED on pin 11
// Teensy++ 2.0 has the LED on pin 6
// Teensy 3.x / Teensy LC have the LED on pin 13
const int ledPin = 13;

//------------------------------------------------------------------------------
SdFs sd;
cid_t m_cid;
csd_t m_csd;
uint32_t m_eraseSize;
uint32_t m_noSectors;
uint32_t m_ocr;
static ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------

void errorPrint()
{
    if (sd.sdErrorCode())
    {
        cout << F("SD errorCode: ") << hex << showbase;
        printSdErrorSymbol(&Serial, sd.sdErrorCode());
        cout << F(" = ") << int(sd.sdErrorCode()) << endl;
        cout << F("SD errorData = ") << int(sd.sdErrorData()) << endl;
    }
}

void printConfig(const SdioConfig &config)
{
#if DEBUG
    (void)config;
    cout << F("Assuming an SDIO interface.\n");
#endif
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
bool cidDmp()
{
#if DEBUG
    cout << F("\nManufacturer ID: ");
    cout << uppercase << showbase << hex << int(m_cid.mid) << dec << endl;
    cout << F("OEM ID: ") << m_cid.oid[0] << m_cid.oid[1] << endl;
    cout << F("Product: ");
    for (uint8_t i = 0; i < 5; i++)
    {
        cout << m_cid.pnm[i];
    }
    cout << F("\nVersion: ");
    cout << int(m_cid.prv_n) << '.' << int(m_cid.prv_m) << endl;
    cout << F("Serial number: ") << hex << m_cid.psn << dec << endl;
    cout << F("Manufacturing date: ");
    cout << int(m_cid.mdt_month) << '/';
    cout << (2000 + m_cid.mdt_year_low + 10 * m_cid.mdt_year_high) << endl;
    cout << endl;
#endif
    return true;
}

//------------------------------------------------------------------------------
bool csdDmp()
{
    bool eraseSingleBlock;
    if (m_csd.v1.csd_ver == 0)
    {
        eraseSingleBlock = m_csd.v1.erase_blk_en;
        m_eraseSize = (m_csd.v1.sector_size_high << 1) | m_csd.v1.sector_size_low;
    }
    else if (m_csd.v2.csd_ver == 1)
    {
        eraseSingleBlock = m_csd.v2.erase_blk_en;
        m_eraseSize = (m_csd.v2.sector_size_high << 1) | m_csd.v2.sector_size_low;
    }
    else
    {
        cout << F("m_csd version error\n");
        return false;
    }
    m_eraseSize++;

    m_noSectors = sdCardCapacity(&m_csd);
#if DEBUG
    cout << F("cardSize: ") << 0.000512 * m_noSectors;
    cout << F(" MB (MB = 1,000,000 bytes)\n");
    cout << F("sectors: ") << m_noSectors << endl;
    cout << F("flashEraseSize: ") << int(m_eraseSize) << F(" blocks\n");
    cout << F("eraseSingleBlock: ");
    if (eraseSingleBlock)
    {
        cout << F("true\n");
    }
    else
    {
        cout << F("false\n");
    }
#endif
    return true;
}
//------------------------------------------------------------------------------

void printCardType()
{
#if DEBUG
    cout << F("\nCard type: ");

    switch (sd.card()->type())
    {
    case SD_CARD_TYPE_SD1:
        cout << F("SD1\n");
        break;

    case SD_CARD_TYPE_SD2:
        cout << F("SD2\n");
        break;

    case SD_CARD_TYPE_SDHC:
        if (sdCardCapacity(&m_csd) < 70000000)
        {
            cout << F("SDHC\n");
        }
        else
        {
            cout << F("SDXC\n");
        }
        break;

    default:
        cout << F("Unknown\n");
    }
#endif
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
bool mbrDmp()
{
    MbrSector_t mbr;
    bool valid = true;
    if (!sd.card()->readSector(0, (uint8_t *)&mbr))
    {
        cout << F("\nread MBR failed.\n");
        return false;
    }
#if DEBUG
    cout << F("\nSD Partition Table\n");
    cout << F("part,boot,bgnCHS[3],type,endCHS[3],start,length\n");
    for (uint8_t ip = 1; ip < 5; ip++)
    {
        MbrPart_t *pt = &mbr.part[ip - 1];
        if ((pt->boot != 0 && pt->boot != 0X80) ||
            getLe32(pt->relativeSectors) > sdCardCapacity(&m_csd))
        {
            valid = false;
        }
        cout << int(ip) << ',' << uppercase << showbase << hex;
        cout << int(pt->boot) << ',';
        for (int i = 0; i < 3; i++)
        {
            cout << int(pt->beginCHS[i]) << ',';
        }
        cout << int(pt->type) << ',';
        for (int i = 0; i < 3; i++)
        {
            cout << int(pt->endCHS[i]) << ',';
        }
        cout << dec << getLe32(pt->relativeSectors) << ',';
        cout << getLe32(pt->totalSectors) << endl;
    }
    if (!valid)
    {
        cout << F("\nMBR not valid, assuming Super Floppy format.\n");
    }
#endif
    return true;
}
//------------------------------------------------------------------------------

void sdInfo()
{
    uint32_t t = millis();

    printConfig(SD_CONFIG);

    if (!sd.cardBegin(SD_CONFIG))
    {
        cout << F(
            "\nSD initialization failed.\n"
            "Do not reformat the card!\n"
            "Is the card correctly inserted?\n"
            "Is there a wiring/soldering problem?\n");
        if (isSpi(SD_CONFIG))
        {
            cout << F(
                "Is SD_CS_PIN set to the correct value?\n"
                "Does another SPI device need to be disabled?\n");
        }
        errorPrint();
        return;
    }
    t = millis() - t;
#if DEBUG
    cout << F("init time: ") << t << " ms" << endl;
#endif
    if (!sd.card()->readCID(&m_cid) ||
        !sd.card()->readCSD(&m_csd) ||
        !sd.card()->readOCR(&m_ocr))
    {
        cout << F("readInfo failed\n");
        errorPrint();
        return;
    }
    printCardType();

    cidDmp();
    csdDmp();
#if DEBUG
    cout << F("\nOCR: ") << uppercase << showbase;
    cout << hex << m_ocr << dec << endl;
#endif
    mbrDmp();
}

void dumpSdCardToSerial()
{
#if DEBUG
    cout << F("Sector to read: ") << m_noSectors << endl;
#endif
    // Send the reader how many bytes to expect
    const uint32_t bytesToWrite = m_noSectors * 512;
    const uint32_t chunkSize = 420;

    Serial.write((uint8_t*)&bytesToWrite, 4);

    for (uint32_t i = 0; i < m_noSectors; i += chunkSize)
    {
        digitalWrite(ledPin, HIGH); // set the LED on

        uint8_t buffer[512 * chunkSize];
        sd.card()->readSectors(i, buffer, chunkSize);

        Serial.write(buffer, sizeof(buffer));

        digitalWrite(ledPin, LOW); // set the LED off

#if DEBUG
        cout << "Read " << i << "/" << m_noSectors << endl;
#endif
    }

#if DEBUG
    cout << F("Done!") << endl;
#endif

    Serial.end();

    digitalWrite(ledPin, LOW); // set the LED on
}

void setup()
{
    // initialize the digital pin as an output.
    pinMode(ledPin, OUTPUT);

    pinMode(2, OUTPUT); // frequency is kbytes/sec

    Serial.begin(115200);
    // Wait for USB Serial

    digitalWrite(ledPin, HIGH); // set the LED on

    while (!Serial)
    {
        SysCall::yield();
    }
    digitalWrite(ledPin, LOW); // set the LED off

    sdInfo();

#if DEBUG
    // Available memory
    cout << F("Free stack: ") << FreeStack() << endl;
#endif

    dumpSdCardToSerial();
}

void loop()
{
}
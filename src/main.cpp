#include <Arduino.h>
#include <SdFs.h>
#include <Ethernet.h>
#include <FreeStack.h>

/*
  Set DISABLE_CS_PIN to disable a second SPI device.
  For example, with the Ethernet shield, set DISABLE_CS_PIN
  to 10 to disable the Ethernet controller.
*/
const int8_t DISABLE_CS_PIN = 10;
/*
  Change the value of SD_CS_PIN if you are using SPI
  and your hardware does not use the default value, SS.  
  Common values are:
  Arduino Ethernet shield: pin 4
  Sparkfun SD shield: pin 8
  Adafruit SD shields and modules: pin 10
*/
const uint8_t SD_CS_PIN = 4;

// Try to select the best SD card configuration.
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI)

// To send EOF and other things
#define CTRL(x) ('x' & 0x1F)

//------------------------------------------------------------------------------
SdFs sd;
cid_t m_cid;
csd_t m_csd;
uint32_t m_eraseSize;
uint32_t m_noSectors;
uint32_t m_ocr;
static ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// telnet defaults to port 23
EthernetServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously
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

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
bool cidDmp()
{
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
    return true;
}
//------------------------------------------------------------------------------

void printCardType()
{
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
    return true;
}
//------------------------------------------------------------------------------

void sdInfo()
{
    uint32_t t = millis();

    cout << F("------------------SD-------------------\n");

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
    cout << F("init time: ") << t << " ms" << endl;

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
    cout << F("\nOCR: ") << uppercase << showbase;
    cout << hex << m_ocr << dec << endl;

    mbrDmp();

    cout << F("---------------------------------------\n");
}

void startEthernetServer()
{
    // initialize the ethernet device
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
    // start listening for clients
    server.begin();

    cout << F("-------------SERVER---------------------\n");
    cout << F("Server address:");
    Serial.print(Ethernet.localIP());
    cout << F("\n----------------------------------------\n");
}

void setup()
{
    Serial.begin(9600);
    // Wait for USB Serial
    while (!Serial)
    {
        SysCall::yield();
    }

    sdInfo();

    startEthernetServer();

    // Available memory
    cout << F("Free stack: ") << FreeStack() << endl;
}

void runServer()
{
    // wait for a new client:
    EthernetClient client = server.available();

    // when the client sends the first byte, say hello:
    if (client)
    {
        if (!alreadyConnected)
        {
            // clear out the input buffer:
            client.flush();
            cout << F("We have a new client") << endl;
            alreadyConnected = true;
        }

        // Send raw sd card data
        cout << F("Reading ") << m_noSectors << F(" sectors\n");
        for (uint32_t i = 0; i < m_noSectors; i++)
        {
            uint8_t rawData[512];
            sd.card()->readSectors(i, rawData, 1);
            server.write(rawData, sizeof(rawData));

            cout << F("Read ") << setprecision(4) << float(i) / float(m_noSectors) << F(" %\r");
        }
        cout << endl;

        client.stop();
    }

}

void loop()
{
    runServer();
}
#include <SPI.h>
#include <X2CAN.h>

/*
  X2CAN – 3.3V MCU + TXS0108E + 5V MCP2515 demo

  Scenario:
  - 3.3V MCU (ESP32, RP2040 / Pico 2, etc.)
  - MCP2515-based CAN module powered at 5V
  - TXS0108E level shifter between MCU and MCP2515

  TXS0108E is not ideal for very high-speed SPI, so it is recommended to
  run SPI at a relatively low clock (e.g. ~1 MHz) for stability.

  This example:
  - Shows how to configure SPI speed via a boolean flag.
  - Demonstrates basic TX + RX using X2CAN with INT support through TXS0108E.
*/

// Adjust these pins to match your wiring
const uint8_t CAN_CS_PIN  = 5;  // Chip Select from MCU → TXS0108E → MCP2515 CS
const uint8_t CAN_INT_PIN = 4;  // MCP2515 INT → TXS0108E → MCU pin

// X2CAN instance
X2CAN CAN1(CAN_CS_PIN, CAN_INT_PIN);

// Set this to true to use a safer, lower SPI speed (recommended with TXS0108E)
bool useLowSpiSpeed = true;

// Configure SPI clock based on board and useLowSpiSpeed flag
void setupSpiClock()
{
    if (useLowSpiSpeed) {
        // SAFE / LOW SPI SPEED (about 1 MHz)
        #if defined(ARDUINO_ARCH_RP2040)
            // Raspberry Pi Pico / Pico 2 (RP2040 / RP2350 based cores)
            SPI.setCLK(1000000);  // 1 MHz
        #elif defined(ARDUINO_ARCH_ESP32)
            // ESP32-family: we use a transaction just to apply settings
            SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
            SPI.endTransaction();
        #elif defined(ARDUINO_ARCH_AVR)
            // Typical AVR @16 MHz → 1 MHz clock
            SPI.setClockDivider(SPI_CLOCK_DIV16);
        #else
            // Generic fallback: try 1 MHz if the core supports SPISettings
            #if defined(SPI_HAS_TRANSACTION) || defined(ARDUINO_ARCH_SAMD)
                SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
                SPI.endTransaction();
            #endif
        #endif
    } else {
        // HIGHER SPI SPEED (around 4 MHz, may be unstable with TXS0108E on some boards)
        #if defined(ARDUINO_ARCH_RP2040)
            SPI.setCLK(4000000);  // 4 MHz
        #elif defined(ARDUINO_ARCH_ESP32)
            SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
            SPI.endTransaction();
        #elif defined(ARDUINO_ARCH_AVR)
            SPI.setClockDivider(SPI_CLOCK_DIV4);  // 16 MHz / 4 = 4 MHz
        #else
            #if defined(SPI_HAS_TRANSACTION) || defined(ARDUINO_ARCH_SAMD)
                SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
                SPI.endTransaction();
            #endif
        #endif
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println();
    Serial.println(F("X2CAN 3.3V + TXS0108E demo"));
    Serial.print(F("Using low SPI speed: "));
    Serial.println(useLowSpiSpeed ? F("YES") : F("NO"));

    // Initialize SPI on the 3.3V MCU
    SPI.begin();

    // Configure SPI clock according to our flag
    setupSpiClock();

    // Initialize X2CAN (MCP2515 @ 16 MHz, CAN @ 500 kbps)
    INT8U status = CAN1.begin(CAN_SPEED_500KBPS_16MHZ);

    if (status != CAN_OK) {
        Serial.print(F("X2CAN init failed, status = "));
        Serial.println(status);
        Serial.println(F("Check:"));
        Serial.println(F("  - TXS0108E wiring (VCCA=3.3V, VCCB=5V, OE=HIGH)"));
        Serial.println(F("  - SPI speed (try useLowSpiSpeed = true)"));
        while (1) {
            delay(1000);
            Serial.println(F("Initialization error – fix wiring or SPI speed."));
        }
    }

    Serial.println(F("X2CAN init OK on 3.3V + TXS0108E"));

    // Optional: configure masks/filters here if needed
}

uint32_t lastTx = 0;

void loop() {
    // Periodic TX: send one frame every 1000 ms
    if (millis() - lastTx >= 1000) {
        lastTx = millis();

        uint8_t data[8] = {0x33, 0x3A, 0x3B, 0x3C, 0x00, 0x00, 0x00, 0x00};
        INT8U txStatus = CAN1.sendMsgBuf(0x123, 0, 8, data);

        Serial.print(F("TX 0x123 status: "));
        Serial.println(txStatus == CAN_OK ? F("OK") : F("FAIL"));
    }

    // RX using MCP2515 INT through TXS0108E
    if (CAN1.checkReceive() == CAN_MSGAVAIL) {
        INT8U len;
        INT8U buf[8];

        if (CAN1.readMsgBuf(&len, buf) == CAN_OK) {
            INT32U id = CAN1.getCanId();

            Serial.print(F("RX ID=0x"));
            Serial.print(id, HEX);
            Serial.print(F(" DLC="));
            Serial.print(len);
            Serial.print(F(" DATA="));

            for (INT8U i = 0; i < len; i++) {
                Serial.print(F(" 0x"));
                Serial.print(buf[i], HEX);
            }
            Serial.println();
        }
    }
}

#include "ClockwiseCaliper.h"
#include "SoftSPISlave.h"

const uint8_t CLK_PIN = 3; // Serial clock pin (must support interrupts)
const uint8_t DATA_PIN = 2; // Serial data pin (must support interrupts)
const uint32_t BIT_MAX_DELAY = 10; // Milliseconds - Maximum time between spi clock pulses
const uint32_t PACKET_MAX_DELAY = 10; // Milliseconds - Maximum time between 8 bit packets
const uint8_t FULL_PACKET_LEN = 3; // Byte length of an entire packet
const uint8_t DATA_LED_PIN = 9; // LED to blink when data is received
const bool DATA_LED_ON_STATE = HIGH; // State to use when data LED is ON


uint32_t lastSpiDataTime = 0;
uint8_t spiDataIndex = 0;
uint8_t resyncCounter = 0;

ClockwiseCaliper caliper;
SoftSPISlave softSpi(CLK_PIN, -1, DATA_PIN, -1);


void printBin(byte aByte) {
    for (int8_t aBit = 7; aBit >= 0; aBit--)
        Serial.write(bitRead(aByte, aBit) ? '1' : '0');
}


void setup() {
    Serial.begin(115200);

    while (!Serial) {
        ; // Wait for serial port to connect. Needed for native USB
    }

    pinMode(DATA_LED_PIN, OUTPUT);
    digitalWrite(DATA_LED_PIN, !DATA_LED_ON_STATE);

    softSpi.begin(false, SSPI_MODE1, SSPI_LSB_FIRST, BIT_MAX_DELAY);

    Serial.println("Starting...");
}


void loop() {
    if (softSpi.rxHasLostData()) {
        // Empty RX buffer, we don't know which byte is which
        while (softSpi.rxBytesAvailable() > 0) {
            softSpi.read();
        }
    }

    while (softSpi.rxHasData()) {
        uint8_t spiData = ~ softSpi.read(); // Invert all bits
        uint32_t currentTime = millis();

        digitalWrite(DATA_LED_PIN, DATA_LED_ON_STATE);

        if (spiDataIndex > 0 && currentTime - lastSpiDataTime >= PACKET_MAX_DELAY) {
            // Time between packets was too long, resync
            resyncCounter++;
            spiDataIndex = 0;
        }

        caliper.updateByte(spiData, spiDataIndex);
        spiDataIndex++;
        lastSpiDataTime = currentTime;

        if (spiDataIndex >= caliper.getPacketLength()) {
            // Completed packet, reset and update
            spiDataIndex = 0;

            caliper.setNewData();
            digitalWrite(DATA_LED_PIN, !DATA_LED_ON_STATE);
        }
    }

    if (caliper.isNewData()) {
        caliper.refershData();

        Serial.print("Resync,");
        Serial.print(resyncCounter); Serial.print(",");
        Serial.print("SpiResync,");
        Serial.print(softSpi.getResyncCount()); Serial.print(",");

        Serial.print("Flags,");
        Serial.print(caliper.getUnit() == MILLIMETERS ? "mm" : "in"); Serial.print(",");
        Serial.print(caliper.getSign() == POSITIVE ? "+" : "-"); Serial.print(",");
        Serial.print("Data,");
        Serial.print(caliper.getRawMeasurement()); Serial.print(",");
        Serial.print(caliper.getMeasurement().measurement, 4); Serial.print(",");

        Serial.println();
    }
}
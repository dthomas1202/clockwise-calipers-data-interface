/*
 * DataInterface.ino - Clockwise Calipers Data Interface
 * Copyright (C) 2025  Diesel Thomas
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * NOTE: To achieve compatibility with mobile devices, such as Apple
 * iDevices, the bMaxPower USB configuration descriptor must be lowered
 * so the device accepts the Arduino. Currently, to do this the line
 *   #define USB_CONFIG_POWER (500)
 * must be changed to less than 100 in USBCore.h of the Arduino core
 * library.
 */


#include "ClockwiseCaliper.h"
#include "SoftSPISlave.h"
#include <HID-Project.h>

// #define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
    #define debug_println(...) Serial.println(__VA_ARGS__)
    #define debug_print(...) Serial.print(__VA_ARGS__)
#else
    #define debug_println(...)
    #define debug_print(...)
#endif


// Caliper SPI like data
const uint8_t CLK_PIN = 3; // Serial clock pin (must support interrupts)
const uint8_t DATA_PIN = 2; // Serial data pin (must support interrupts)
const uint32_t BIT_MAX_DELAY = 10; // Milliseconds - Maximum time between spi clock pulses
const uint32_t PACKET_MAX_DELAY = 10; // Milliseconds - Maximum time between 8 bit packets

// DIP switch control pins
const uint8_t DIP_CTRL_A_PIN = 16; // Type CTRL+A before the measurement
const uint8_t DIP_UNITS_PIN = 14; // Type the units after the measurement
const uint8_t DIP_NEWLINE_PIN = 15; // Type a newline after the measurement
const uint8_t DIP_TAB_PIN = A0; // Type a tab after the measurement
const uint8_t DIP_COMMA_PIN = A1; // Type a comma after the measurement
const uint8_t DIP_SPACE_PIN = A2; // Type a space after the measurement
// Any of newline, tab, comma, and space can be combined to in that order
const bool DIP_ON_STATE = LOW; // Switch is active when it is in this state

const uint8_t TRIGGER_PIN = 7; // Triggers typing the measurement (should support interrupts)
const uint8_t TRIGGER_PIN_INT_MODE = FALLING; // When to trigger the interrupt (either RISING or FALLING)
// NOTE: You can rewrite this for TRIGGER_PIN to not require interrupts, its just real convenient

const uint8_t DATA_LED_PIN = A3; // LED to blink when data is r * <one line to give the program's name and a brief idea of what it does.>
const bool DATA_LED_ACTIVE_STATE = HIGH; // State to use when data LED is ON

const uint8_t BUZZER_PIN = 10; // Passive buzzer to sound when data is typed (must support pwm)
const uint16_t BUZZER_FREQ = 4000; // Hz - Frequency of buzzer
const uint16_t BUZZER_DURATION = 10; // Milliseconds - Time to sound the buzzer for


uint8_t resyncCounter = 0;
volatile bool triggerFlag = false;

ClockwiseCaliper caliper;
SoftSPISlave softSpi(CLK_PIN, -1, DATA_PIN, -1);


void triggerIsr() {
    triggerFlag = true;
}


void setup() {
    #ifdef DEBUG_SERIAL
        Serial.begin(115200);

        while (!Serial && millis() < 2000) {
            ; // Wait for serial port to connect, or 2 seconds. Needed for native USB
        }
    #endif

    softSpi.begin(false, SSPI_MODE1, SSPI_LSB_FIRST, BIT_MAX_DELAY);

    // Override pinMode from softSpi to enable pullups
    pinMode(CLK_PIN, INPUT_PULLUP);
    pinMode(DATA_PIN, INPUT_PULLUP);

    pinMode(DIP_CTRL_A_PIN, INPUT_PULLUP);
    pinMode(DIP_UNITS_PIN, INPUT_PULLUP);
    pinMode(DIP_NEWLINE_PIN, INPUT_PULLUP);
    pinMode(DIP_TAB_PIN, INPUT_PULLUP);
    pinMode(DIP_COMMA_PIN, INPUT_PULLUP);
    pinMode(DIP_SPACE_PIN, INPUT_PULLUP);

    pinMode(TRIGGER_PIN, INPUT_PULLUP);

    pinMode(DATA_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(DATA_LED_PIN, !DATA_LED_ACTIVE_STATE);

    BootKeyboard.begin();

    debug_println("Starting...");

    tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_DURATION);
    delay(200);
    tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_DURATION);

    // Set last so pins stabilize
    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), triggerIsr, TRIGGER_PIN_INT_MODE);
}


void emptySoftSpiBuffer() {
    // Empty RX buffer, we don't know which byte is which
    while (softSpi.rxBytesAvailable() > 0) {
        softSpi.read();
    }
}


void recievePacket() {
    static uint8_t spiDataIndex = 0;
    static uint32_t lastSpiDataTime = 0;

    uint8_t spiData = ~ softSpi.read(); // Invert all bits
    uint32_t currentTime = millis();

    digitalWrite(DATA_LED_PIN, DATA_LED_ACTIVE_STATE);

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
        digitalWrite(DATA_LED_PIN, !DATA_LED_ACTIVE_STATE);
    }
}


void typeIfPin(KeyboardKeycode key, uint8_t pin) {
    if (digitalRead(pin) == DIP_ON_STATE) {
        BootKeyboard.write(key);
    }
}


void typeMeasurement() {
    // CTRL + A
    if (digitalRead(DIP_CTRL_A_PIN) == DIP_ON_STATE) {
        BootKeyboard.press(KEY_LEFT_CTRL);
        BootKeyboard.press(KEY_A);
        BootKeyboard.releaseAll();
    }

    // Type measurement
    BootKeyboard.print(caliper.getMeasurement(), 4);

    // Units
    if (digitalRead(DIP_UNITS_PIN) == DIP_ON_STATE) {
        BootKeyboard.print(caliper.getUnitString());
    }

    typeIfPin(KEY_ENTER, DIP_NEWLINE_PIN); // Newline
    typeIfPin(KEY_TAB, DIP_TAB_PIN);       // Tab
    typeIfPin(KEY_COMMA, DIP_COMMA_PIN);   // Comma
    typeIfPin(KEY_SPACE, DIP_SPACE_PIN);   // Space

    tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_DURATION);
}


void loop() {
    if (softSpi.rxHasLostData()) {
        emptySoftSpiBuffer();
        debug_println("Lost data, buffer emptied");
    }

    while (softSpi.rxHasData()) {
        recievePacket();
    }

    if (caliper.isNewData()) {
        // Always maintain most recent data
        caliper.refershData();

        debug_print("resync: ");     debug_print(resyncCounter);            debug_print(", ");
        debug_print("spi_resync: "); debug_print(softSpi.getResyncCount()); debug_print(", ");

        debug_print("unit: "); debug_print(caliper.getUnitString());     debug_print(", ");
        debug_print("sign: "); debug_print(caliper.getSignString());     debug_print(", ");
        debug_print("raw: ");  debug_print(caliper.getRawMeasurement()); debug_print(", ");

        debug_print("measurement: "); debug_print(caliper.getMeasurement(), 4); debug_println();
    }

    if (triggerFlag) {
        triggerFlag = false;
        typeMeasurement();
        debug_println("Typed measurement");
    }
}
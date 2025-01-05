/*
 * ClockwiseCaliper.h - Clockwise Caliper Serial Data Decoder (Header File)
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


#pragma once
#include <stdint.h>


constexpr char EMPTY_STR[] = "";
constexpr char MILLIMETERS_STR[] = "mm";
constexpr char INCHES_STR[] = "in";
constexpr char POSITIVE_STR[] = "+";
constexpr char NEGATIVE_STR[] = "-";

/**
 * Union representation of the a 24-bit caliper data packet.
 */
typedef union {
    uint32_t integer; // Representation as a 32-bit integer

    uint8_t array[3]; // Representation as an array of 3 8-bit integers

    struct {
        uint8_t lsb; // Least significant
        uint8_t mb;  // Middle byte
        uint8_t msb; // Most significant
    } bytes; // Representation in bytes (8-bits) format

    struct {
        uint32_t measurement:20;    // Measurement data (hundredths of a millimeter, or 1/2 thousandths of an inch)
        uint8_t sign:1;             // Measurement sign (0: positive, 1: negative)
        uint8_t :1;                 // Unknown use
        uint8_t :1;                 // Unknown use
        uint8_t unit:1;             // Measurement unit (0: millimeters, 1: inches)
    } data;
} caliper_data_t;

typedef enum : uint8_t {
    MILLIMETERS = 0,
    INCHES = 1
} caliper_unit_t;

typedef enum : uint8_t {
    POSITIVE = 0,
    NEGATIVE = 1
} caliper_sign_t;


class ClockwiseCaliper {
public:
    ClockwiseCaliper();

    void updateMsb(uint8_t msb); // Updates the most significant byte of data.
    void updateMb(uint8_t mb);   // Updates the middle byte of data.
    void updateLsb(uint8_t lsb); // Updates the least significant byte of data.
    void updateByte(uint8_t byte, uint8_t index); // Updates the byte at the given index.
    void updateDataBytes(uint8_t msb, uint8_t mb, uint8_t lsb); // Updates the most significant, middle, and least significant bytes of data.

    void refershData(); // Updates the readable data with the most recent data.

    uint32_t getRawMeasurement(); // Returns the current absolute, unconverted 20-bit measurement.
    float getMeasurement();       // Returns the converted measurement.

    caliper_unit_t getUnit(); // Returns the current measurement unit.
    char* getUnitString();    // Returns the current measurement unit as a string.
    caliper_sign_t getSign(); // Returns the current measurement sign.
    char* getSignString();    // Returns the current measurement sign as a string.

    void setNewData();   // Sets the newData flag.
    void clearNewData(); // Clears the newData flag.
    bool isNewData();    // Returns the status of the newData flag.

    uint8_t getPacketLength() const; // Returns the length of a full data packet in bytes.

private:
    void swapReadWrite(); // Swaps the caliper data read and write pointers.

    bool newData; // Data was changed in *pWriteCaliperData

    caliper_data_t *pWriteCaliperData; // Points to structure to use for new data
    caliper_data_t *pReadCaliperData;  // Points to structure to use for getting data
    caliper_data_t caliperDataA;
    caliper_data_t caliperDataB;
};

/*
 * ClockwiseCaliper.cpp - Clockwise Caliper Serial Data Decoder
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


#include "ClockwiseCaliper.h"


/**
 * Clockwise Caliper constructor.
 */
ClockwiseCaliper::ClockwiseCaliper() {
    this->newData = false;
    this->pWriteCaliperData = &this->caliperDataA;
    this->pReadCaliperData = &this->caliperDataB;
}


/**
 * Updates the most significant byte of data.
 *
 * @param msb most significant byte
 */
void ClockwiseCaliper::updateMsb(uint8_t msb) {
    this->pWriteCaliperData->bytes.msb = msb;
}

/**
 * Updates the middle byte of data.
 *
 * @param mb middle byte
 */
void ClockwiseCaliper::updateMb(uint8_t mb) {
    this->pWriteCaliperData->bytes.mb = mb;
}

/**
 * Updates the least significant byte of data.
 *
 * @param lsb least significant byte
 */
void ClockwiseCaliper::updateLsb(uint8_t lsb) {
    this->pWriteCaliperData->bytes.lsb = lsb;
}

/**
 * Updates the byte at the given index.
 * Index 0 is the least significant byte, 1 is the middle byte, and 2
 * is the most significant byte.
 *
 * @param byte new byte data
 * @param index index to place it in
 */
void ClockwiseCaliper::updateByte(uint8_t byte, uint8_t index) {
    this->pWriteCaliperData->array[index] = byte;
}

/**
 * Updates the most significant, middle, and least significant bytes of data.
 * Also sets the newData flag.
 *
 * @param msb most significant byte
 */
void ClockwiseCaliper::updateDataBytes(uint8_t msb, uint8_t mb, uint8_t lsb) {
    this->pWriteCaliperData->bytes.msb = msb;
    this->pWriteCaliperData->bytes.mb = mb;
    this->pWriteCaliperData->bytes.lsb = lsb;
    this->setNewData();
}


/**
 * Updates the readable data with the most recent data.
 * Should be called just before reading data.
 * Data is guaranteed to be consistent between calls to refershData().
 * Also clears the newData flag.
 */
void ClockwiseCaliper::refershData() {
    this->clearNewData();
    this->swapReadWrite();
}


/**
 * Returns the current absolute, unconverted 20-bit measurement.
 *
 * @return the current raw measurement data
 */
uint32_t ClockwiseCaliper::getRawMeasurement() {
    return this->pReadCaliperData->data.measurement;
}

/**
 * Returns the converted measurement.
 * Conversion will be done to whichever unit is selected on the calipers.
 * The measurement may be positive or negative depending on the sign.
 *
 * @return the converted measurement
 */
float ClockwiseCaliper::getMeasurement() {
    float measurement = this->getRawMeasurement();

    switch (this->getUnit()) {
        case MILLIMETERS:
            measurement /= 100; // From hundredths of a millimeter
            break;

        case INCHES:
            measurement /= 2000; // From 1/2 thousandths of an inch
            break;

        default:
            ; // Unknown unit, do nothing
    }

    if (this->getSign() == NEGATIVE) {
        measurement *= -1;
    }

    return measurement;
}


/**
 * Returns the current measurement unit.
 *
 * @return the current measurement unit
 */
caliper_unit_t ClockwiseCaliper::getUnit() {
    return this->pReadCaliperData->data.unit;
}

/**
 * Returns the current measurement unit as a string.
 *
 * @return the current measurement unit as a pointer to a C string
 */
char* ClockwiseCaliper::getUnitString() {
    char *unitString = EMPTY_STR;

    switch (this->getUnit()) {
        case MILLIMETERS:
            unitString = MILLIMETERS_STR;
            break;

        case INCHES:
            unitString = INCHES_STR;
            break;
    }

    return unitString;
}

/**
 * Returns the current measurement sign.
 *
 * @return the current measurement sign
 */
caliper_sign_t ClockwiseCaliper::getSign() {
    return this->pReadCaliperData->data.sign;
}

/**
 * Returns the current measurement sign as a string.
 *
 * @return the current measurement sign as a pointer to a C string
 */
char* ClockwiseCaliper::getSignString() {
    char *signString = POSITIVE_STR;

    if (this->getSign() == NEGATIVE) {
        signString = NEGATIVE_STR;
    }

    return signString;
}


/**
 * Sets the newData flag.
 */
void ClockwiseCaliper::setNewData() {
    this->newData = true;
}

/**
 * Clears the newData flag.
 */
void ClockwiseCaliper::clearNewData() {
    this->newData = false;
}

/**
 * Returns the status of the newData flag.
 * The newData flag is used to indicate that the data has been updated
 * since it was last read.
 *
 * @return true if set, false otherwise
 */
bool ClockwiseCaliper::isNewData() {
    return this->newData;
}

/**
 * Returns the length of a full data packet in bytes.
 * Always 3.
 *
 * @return the length of a full data packet in bytes.
 */
uint8_t ClockwiseCaliper::getPacketLength() const {
    return 3;
}


/**
 * Swaps the caliper data read and write pointers.
 */
void ClockwiseCaliper::swapReadWrite() {
    caliper_data_t *pTemp = this->pReadCaliperData;
    this->pReadCaliperData = this->pWriteCaliperData;
    this->pWriteCaliperData = pTemp;
}

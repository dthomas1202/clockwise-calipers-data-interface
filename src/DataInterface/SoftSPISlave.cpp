#include "SoftSPISlave.h"


/**
 * Software SPI slave constructor.
 * The CLK pin must be defined and must be an interrupt capable pin.
 * MISO, MOSI, and SS pins are optional, however, at least one of MISO
 * or MOSI must be defined. Also, if MISO is defined then SS must also
 * be defined and must be an interrupt capable pin.
 *
 * @param clkPin serial clock pin
 * @param misoPin master in, slave out pin
 * @param mosiPin master out, slave in pin
 * @param ssPin slave select pin
 */
SoftSPISlave::SoftSPISlave(int16_t clkPin,
                           int16_t misoPin = -1,
                           int16_t mosiPin = -1,
                           int16_t ssPin = -1) {
    this->clkPin = clkPin;
    this->misoPin = misoPin;
    this->mosiPin = mosiPin;
    this->ssPin = ssPin;


    this->dataIndex = 0;
    this->resyncCount = 0;
    this->rxDataLost = false;
}


/**
 *
 */
void SoftSPISlave::begin(bool ssActiveHigh = false,
                         softspi_mode_t spiMode = SSPI_MODE0,
                         softspi_data_order_t spiDataOrder = SSPI_MSB_FIRST,
                         uint32_t maxClkTime = 0) {
    this->ssActiveHigh = ssActiveHigh;
    this->spiCPHA = SoftSPISlave::getBit(spiMode, 0);
    this->spiCPOL = SoftSPISlave::getBit(spiMode, 1);
    this->spiDataOrder = spiDataOrder;
    this->maxClkTime = maxClkTime;

    int16_t clkIntPin = digitalPinToInterrupt(this->clkPin);
    int16_t ssIntPin = digitalPinToInterrupt(this->ssPin);

    if (clkIntPin < 0 // CLK pin is not set, or not an interrupt capable pin
            || (this->misoPin < 0 && this->mosiPin < 0) // At least one of MISO or MOSI is not set
            || (this->misoPin >= 0 && ssIntPin < 0)) { // MISO is defined and SS is not defined or not interrupt capable
        return;
    }

    pinMode(this->clkPin, INPUT);

    if (this->mosiPin >= 0) {
        pinMode(this->mosiPin, INPUT);
    }

    if (this->ssPin >= 0) {
        // SS is defined
        pinMode(this->ssPin, INPUT);

        if (this->misoPin >= 0 && ssIntPin >= 0) {
            // MISO is defined and SS is interrupt capable
            attachInterrupt(ssIntPin, bindArgGateThisAllocate(&SoftSPISlave::ssIsr, this), CHANGE);
        }
    }

    attachInterrupt(clkIntPin, bindArgGateThisAllocate(&SoftSPISlave::clkIsr, this), CHANGE);
}

/**
 *
 */
void SoftSPISlave::end() {
    int16_t clkIntPin = digitalPinToInterrupt(this->clkPin);
    int16_t ssIntPin = digitalPinToInterrupt(this->ssPin);

    detachInterrupt(clkIntPin);

    if (this->misoPin >= 0 && ssIntPin >= 0) {
        // SS would have been setup with interrupts
        detachInterrupt(ssIntPin);
    }

    // Only MISO is ever an output
    pinMode(this->misoPin, INPUT);
}


/**
 * Returns the number of bytes that can be read.
 *
 * @return the number of bytes that can be read
 */
uint8_t SoftSPISlave::rxBytesAvailable() {
    return this->rxBuff.size();
}

/**
 * Returns the remaining number of bytes that can be received without
 * overwriting.
 *
 * @return the remaining space in the RX buffer
 */
uint8_t SoftSPISlave::rxBytesRemaining() {
    return this->rxBuff.available();
}

/**
 *
 */
bool SoftSPISlave::rxHasData() {
    return !this->rxBuff.isEmpty();
}

/**
 * Returns true if data has been lost since the last time this was called.
 *
 * @return true if data has been lost
 */
bool SoftSPISlave::rxHasLostData() {
    bool retVal = this->rxDataLost;

    this->rxDataLost = false;

    return retVal;
}

/**
 *
 */
uint8_t SoftSPISlave::read() {
    return this->rxBuff.shift();
}

/**
 *
 */
uint8_t SoftSPISlave::peek() {
    return this->rxBuff.first();
}


/**
 *
 */
uint8_t SoftSPISlave::txBytesAvailable() {
    return this->txBuff.available();
}

/**
 *
 */
bool SoftSPISlave::txIsFull() {
    return this->txBuff.isFull();
}

/**
 *
 */
void SoftSPISlave::write(uint8_t data) {
    this->txBuff.push(data);
}

/**
 * Returns the count of timeouts due to maxClkTime.
 */
uint8_t SoftSPISlave::getResyncCount() {
    return this->resyncCount;
}


/**
 *
 */
void SoftSPISlave::clkIsr() {
    // Runs on either RISING or FALLING edge of CLK
    static uint32_t lastClkTime = 0;
    static uint8_t rxData = 0;
    static uint8_t txData = 0;

    // Return if SS is not active
    if (this->ssPin >= 0 && digitalRead(this->ssPin) == this->ssActiveHigh) {
        this->dataIndex = 0;
        return;
    }

    // Sampling when going LOW to HIGH (MODE0, MODE3)
    bool clkSample = digitalRead(this->clkPin);
    uint32_t currentTime = millis();
    uint8_t index;

    // Fill txData with something
    if (!this->txBuff.isEmpty() && this->dataIndex <= 0) {
        txData = this->txBuff.shift();

    } else if (this->dataIndex <= 0) {
        // If buffer is empty send all zeros
        txData = 0;
    }

    // Figure out when we are sampling or shifting out
    if (this->spiCPHA != this->spiCPOL) {
        // Sampling when going HIGH to LOW (MODE1, MODE2)
        clkSample = !clkSample;
    }

    // Reset if the time between clock pulses was too long
    if (this->dataIndex > 0
        && this->maxClkTime > 0 // maxClkTime is enabled
        && currentTime - lastClkTime > this->maxClkTime) {
        this->dataIndex = 0;
    this->resyncCount++;
        }

        lastClkTime = currentTime;

        // Send/Receive bits in the correct order
        if (this->spiDataOrder == SSPI_MSB_FIRST) {
            index = 7 - (this->dataIndex / 2);
        } else {
            index = (this->dataIndex / 2);
        }

        if (clkSample && this->mosiPin >= 0) {
            // This is a sampling clock cycle
            bool mosiState = digitalRead(this->mosiPin);

            rxData = SoftSPISlave::setBitTo(rxData, index, mosiState);

        } else if (!clkSample && this->misoPin >= 0) {
            // This is a shift out clock cycle
            if (!this->spiCPHA) { // SPI MODE0 or MODE2
                // Need to set MISO to future bit
                // NOTE The index will be out of bounds on the last clock
                // cycle. This is fine since this bit is never read by the
                // master, and getBit() will just return all zeros
                index++;
            }

            digitalWrite(this->misoPin, SoftSPISlave::getBit(txData, index));
        }

        this->dataIndex++;

        if (this->dataIndex > 15) {
            // Deal with buffers and reset
            // .push() returns false if an overwrite occurred
            this->rxDataLost = !this->rxBuff.push(rxData);
            this->dataIndex = 0;
        }
}

/**
 *
 */
void SoftSPISlave::ssIsr() {
    // Runs on either RISING or FALLING edge of SS, so long as MISO is defined
    bool ssActive = digitalRead(this->ssPin) == this->ssActiveHigh;

    if (ssActive) {
        // We have been select
        // Setup MISO pin for initial cycle
        pinMode(this->misoPin, OUTPUT);

        bool state = false;

        // Only set state if there is something in the buffer
        if (!this->txBuff.isEmpty()) {
            uint8_t startBit = this->spiDataOrder == SSPI_MSB_FIRST ? 7 : 0;
            bool state = SoftSPISlave::getBit(this->txBuff.first(), startBit);
        }

        digitalWrite(this->misoPin, state);

        this->dataIndex = 0;

    } else {
        // We have been deselected
        // Set back to input so other devices can use the line
        pinMode(this->misoPin, INPUT);
    }
}


/**
 * Set bin at index n to the value of x.
 * Index 0 is the least significant bit.
 *
 * @param number byte to change
 * @param n index of the bit
 * @param x new state of the bit
 * @return modified byte
 */
static inline uint8_t SoftSPISlave::setBitTo(uint8_t number, uint8_t n, bool x) {
    return (number & ~((uint8_t)1 << n)) | ((uint8_t)x << n);
}

/**
 * Get bin at index n.
 * Index 0 is the least significant bit.
 *
 * @param number byte to get bit
 * @param n index of the bit
 * @return state of the bit
 */
static inline bool SoftSPISlave::getBit(uint8_t number, uint8_t n) {
    return (number >> n) & 1;
}

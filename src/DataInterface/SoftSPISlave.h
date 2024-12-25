#pragma once

#define CIRCULAR_BUFFER_INT_SAFE

#include <stdint.h>
#include <Arduino.h>
#include <BindArg.h>
#include <CircularBuffer.hpp>


const uint8_t RECEIVE_BUF_SIZE = 8;
const uint8_t TRANSMIT_BUF_SIZE = 8;


typedef enum : uint8_t {
    SSPI_MODE0,
    SSPI_MODE1,
    SSPI_MODE2,
    SSPI_MODE3
} softspi_mode_t;

typedef enum : uint8_t {
    SSPI_MSB_FIRST,
    SSPI_LSB_FIRST
} softspi_data_order_t;

typedef enum : uint8_t {
    SSPI_CLK_ISR,
    SSPI_SS_ISR
} softspi_isr_t;


class SoftSPISlave {
public:
    SoftSPISlave(int16_t clkPin,
                 int16_t misoPin,
                 int16_t mosiPin,
                 int16_t ssPin);

    void begin(bool ssActiveHigh,
               softspi_mode_t spiMode,
               softspi_data_order_t spiDataOrder,
               uint32_t maxClkTime);
    void end();

    // RX
    uint8_t rxBytesAvailable(); // Returns the number of bytes that can be read.
    uint8_t rxBytesRemaining(); // Returns the remaining number of bytes that can be received without overwriting.
    bool rxHasData(); // Returns true if data is available to be read.
    bool rxHasLostData(); // Returns true if data has been lost since the last time this was called.
    uint8_t read();
    uint8_t peek();

    // TX
    uint8_t txBytesAvailable(); // Returns the remaining number of bytes that can be written without blocking.
    bool txIsFull(); // Returns true if the TX buffer is full.
    void write(uint8_t data);

    uint8_t getResyncCount(); // Returns the count of timeouts due to maxClkTime.

private:
    int16_t clkPin; // Serial clock.
    int16_t misoPin; // Serial data slave out.
    int16_t mosiPin; // Serial data slave in.
    int16_t ssPin; // Slave select.
    bool ssActiveHigh;

    bool spiCPHA;
    bool spiCPOL;
    softspi_data_order_t spiDataOrder;

    uint32_t maxClkTime; // Max time between clock pulses, disabled when 0.
    uint8_t dataIndex; // Increments two times each clock (one RISING, one FALLING).

    volatile CircularBuffer<uint8_t, RECEIVE_BUF_SIZE> rxBuff;
    volatile CircularBuffer<uint8_t, TRANSMIT_BUF_SIZE> txBuff;

    volatile uint8_t resyncCount;
    volatile bool rxDataLost;

    void clkIsr();
    void ssIsr();

    static inline uint8_t setBitTo(uint8_t number, uint8_t n, bool x); // Set bin at index n to the value of x.
    static inline bool getBit(uint8_t number, uint8_t n); // Get bin at index n.
};
#ifndef _PERIPHERAL_SPI_H_
#define _PERIPHERAL_SPI_H_

#include <map>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/sync.h>
#include <hardware/platform_defs.h>

#if __has_include("BoardConfig.h")
#include "BoardConfig.h"
#endif

#define DMA_BUFFER_SIZE 2048 // Each direction gets a 2k buffer

typedef enum {
  SPI_MODE0 = 0,
  SPI_MODE1 = 1,
  SPI_MODE2 = 2,
  SPI_MODE3 = 3,
} SPIMode;

#ifndef SPI0_ENABLED
#define SPI0_ENABLED 0
#endif

#ifndef SPI0_PIN_RX
#define SPI0_PIN_RX -1
#endif

#ifndef SPI0_PIN_CS
#define SPI0_PIN_CS -1
#endif

#ifndef SPI0_PIN_SCK
#define SPI0_PIN_SCK -1
#endif

#ifndef SPI0_PIN_TX
#define SPI0_PIN_TX -1
#endif

#ifndef SPI1_ENABLED
#define SPI1_ENABLED 0
#endif

#ifndef SPI1_PIN_RX
#define SPI1_PIN_RX -1
#endif

#ifndef SPI1_PIN_CS
#define SPI1_PIN_CS -1
#endif

#ifndef SPI1_PIN_SCK
#define SPI1_PIN_SCK -1
#endif

#ifndef SPI1_PIN_TX
#define SPI1_PIN_TX -1
#endif

class PeripheralSPI {
public:
    PeripheralSPI();

    ~PeripheralSPI() {
        deselect();
        deactivate();
    }

    bool configured = false;
    bool initialized = false;

    spi_inst_t* getController() { return _SPI; }
    bool hasChipSelect() const {
        return _CS >= 0 && static_cast<unsigned int>(_CS) < NUM_BANK0_GPIOS;
    }

    // Set the configuration for this SPI peripheral instance
    void setConfig(uint8_t block, uint8_t tx, uint8_t rx, uint8_t sck, uint8_t cs);

    // Disable SPI instance and all associated system resources (such as DMA channels) associated with that instance
    void deactivate();

    // Conditionally transfers and/or receives data of the provided length
    void transfer(const uint8_t *tx, uint8_t *rx, size_t count);

    // Reads a byte of data from the SPI bus while sending the transfer value provided
    uint8_t transfer(uint8_t tx);

    // Reads two bytes of data from the SPI bus while sending the transfer value provided
    uint16_t transfer16(uint16_t tx);

    // Activate CS pin for this SPI peripheral instance
    void select(int8_t cs = -1);

    // Deactivate currently active CS pin for this SPI peripheral instance
    void deselect();

    // Begin a SPI transaction
    void beginTransaction(uint32_t speedMHz, spi_order_t bitOrder, SPIMode spiMode);

    // End a SPI transaction
    void endTransaction();

private:
    const uint32_t SPI_DEFAULT_SPEED = 1000000; // 1Mhz

    uint8_t _RX = 0xFF; // MISO/POCI gpio pin
    uint8_t _TX = 0xFF; // MOSI/PICO gpio pin
    uint8_t _SCK = 0xFF;
    int _CS = -1; // The HW CS pin
    int _CSActive = -1; // The active CS pin, negative value indicates no selected pin

    spi_inst_t *_SPI = nullptr; // The RP2040 SPI instance
    uint32_t _Speed = SPI_DEFAULT_SPEED;
    SPIMode _SpiMode = SPI_MODE0;
    spi_order_t _BitOrder = SPI_MSB_FIRST;
    spi_cpol_t _Cpol = SPI_CPOL_0;
    spi_cpha_t _Cpha = SPI_CPHA_0;

    bool _UseDMA = false;
    int _dmaRxChannel = -1;
    int _dmaTxChannel = -1;
    uint8_t *_dmaRxBuf = nullptr;
    uint8_t *_dmaTxBuf = nullptr;

    spi_inst_t* _hardwareBlocks[NUM_SPIS] = {spi0,spi1};

    void setup();

    inline spi_cpol_t get_cpol(SPIMode mode) {
        switch (mode)
        {
            case SPI_MODE0:
                return SPI_CPOL_0;
            case SPI_MODE1:
                return SPI_CPOL_0;
            case SPI_MODE2:
                return SPI_CPOL_1;
            case SPI_MODE3:
                return SPI_CPOL_1;
        }

        // Error
        return SPI_CPOL_0;
    }

    inline spi_cpha_t get_cpha(SPIMode mode) {
        switch (mode)
        {
            case SPI_MODE0:
                return SPI_CPHA_0;
            case SPI_MODE1:
                return SPI_CPHA_1;
            case SPI_MODE2:
                return SPI_CPHA_0;
            case SPI_MODE3:
                return SPI_CPHA_1;
        }

        // Error
        return SPI_CPHA_0;
    }
};

#endif

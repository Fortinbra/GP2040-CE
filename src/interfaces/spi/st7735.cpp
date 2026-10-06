#include "interfaces/spi/st7735.h"

void GPGFX_ST7735::init(GPGFX_DisplayTypeOptions options) {
    _displayOptions = options;
    _options = options;

    PeripheralSPI* spi = _displayOptions.spi;
    spi->beginTransaction(24000000, SPI_MSB_FIRST, SPI_MODE0);

    gpio_init(_displayOptions.dcPin);
    gpio_set_dir(_displayOptions.dcPin, GPIO_OUT);
    gpio_put(_displayOptions.dcPin, 1);

    if (_displayOptions.resetPin >= 0) {
        gpio_init(_displayOptions.resetPin);
        gpio_set_dir(_displayOptions.resetPin, GPIO_OUT);
        gpio_put(_displayOptions.resetPin, 0);
        sleep_ms(10);
        gpio_put(_displayOptions.resetPin, 1);
        sleep_ms(120);
    }

    writeCommand(0x01);
    sleep_ms(150);
    writeCommand(0x11);
    sleep_ms(120);

    const uint8_t frameRate[] = {0x01, 0x2C, 0x2D};
    writeCommand(0xB1, frameRate, sizeof(frameRate));
    writeCommand(0xB2, frameRate, sizeof(frameRate));
    const uint8_t inversionControl[] = {0x07};
    writeCommand(0xB4, inversionControl, sizeof(inversionControl));
    const uint8_t powerControl1[] = {0xA2, 0x02, 0x84};
    writeCommand(0xC0, powerControl1, sizeof(powerControl1));
    const uint8_t powerControl2[] = {0xC5};
    writeCommand(0xC1, powerControl2, sizeof(powerControl2));
    const uint8_t powerControl3[] = {0x0A, 0x00};
    writeCommand(0xC2, powerControl3, sizeof(powerControl3));
    const uint8_t powerControl4[] = {0x8A, 0x2A};
    writeCommand(0xC3, powerControl4, sizeof(powerControl4));
    const uint8_t powerControl5[] = {0x8A, 0xEE};
    writeCommand(0xC4, powerControl5, sizeof(powerControl5));
    const uint8_t vcomControl[] = {0x0E};
    writeCommand(0xC5, vcomControl, sizeof(vcomControl));
    uint8_t memoryAccess[] = {static_cast<uint8_t>(0x68 ^ ((options.orientation & 1) << 7) ^ ((options.orientation & 2) << 5))};
    writeCommand(0x36, memoryAccess, sizeof(memoryAccess));
    const uint8_t pixelFormat[] = {0x05};
    writeCommand(0x3A, pixelFormat, sizeof(pixelFormat));
    const uint8_t positiveGamma[] = {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10};
    writeCommand(0xE0, positiveGamma, sizeof(positiveGamma));
    const uint8_t negativeGamma[] = {0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10};
    writeCommand(0xE1, negativeGamma, sizeof(negativeGamma));
    writeCommand(_displayOptions.inverted ? 0x21 : 0x20);
    writeCommand(0x13);
    writeCommand(0x29);
    sleep_ms(100);

    clear();
    drawBuffer(nullptr);
    setPower(true);
}

void GPGFX_ST7735::setPower(bool isPowered) {
    _isPowered = isPowered;
    writeCommand(_isPowered ? 0x29 : 0x28);
}

void GPGFX_ST7735::clear() {
    memset(frameBuffer, 0, sizeof(frameBuffer));
}

uint32_t GPGFX_ST7735::getPixel(uint8_t x, uint8_t y) {
    if (x >= LOGICAL_WIDTH || y >= LOGICAL_HEIGHT)
        return 0;
    return frameBuffer[y * LOGICAL_WIDTH + x];
}

void GPGFX_ST7735::drawPixel(uint8_t x, uint8_t y, uint32_t color) {
    if (x >= LOGICAL_WIDTH || y >= LOGICAL_HEIGHT)
        return;
    frameBuffer[y * LOGICAL_WIDTH + x] = color == 1 ? 0xFFFF : static_cast<uint16_t>(color);
}

void GPGFX_ST7735::drawBuffer(uint8_t* pBuffer) {
    setAddressWindow();
    _displayOptions.spi->select();
    gpio_put(_displayOptions.dcPin, 1);

    uint8_t transferBuffer[512];
    size_t transferLength = 0;
    const uint16_t* pixels = reinterpret_cast<const uint16_t*>(pBuffer);
    for (uint16_t y = 0; y < PANEL_HEIGHT; y++) {
        uint16_t sourceY = (static_cast<uint32_t>(y) * LOGICAL_HEIGHT) / PANEL_HEIGHT;
        for (uint16_t x = 0; x < PANEL_WIDTH; x++) {
            uint16_t sourceX = (static_cast<uint32_t>(x) * LOGICAL_WIDTH) / PANEL_WIDTH;
            uint16_t color = pixels
                ? pixels[sourceY * LOGICAL_WIDTH + sourceX]
                : frameBuffer[sourceY * LOGICAL_WIDTH + sourceX];
            transferBuffer[transferLength++] = color >> 8;
            transferBuffer[transferLength++] = color & 0xFF;
            if (transferLength == sizeof(transferBuffer)) {
                _displayOptions.spi->transfer(transferBuffer, nullptr, transferLength);
                transferLength = 0;
            }
        }
    }

    if (transferLength > 0)
        _displayOptions.spi->transfer(transferBuffer, nullptr, transferLength);

    _displayOptions.spi->deselect();
}

void GPGFX_ST7735::writeCommand(uint8_t command, const uint8_t* data, size_t length) {
    _displayOptions.spi->select();
    gpio_put(_displayOptions.dcPin, 0);
    _displayOptions.spi->transfer(command);
    if (length > 0) {
        gpio_put(_displayOptions.dcPin, 1);
        _displayOptions.spi->transfer(data, nullptr, length);
    }
    _displayOptions.spi->deselect();
}

void GPGFX_ST7735::setAddressWindow() {
    const uint8_t columns[] = {
        0x00, COLUMN_START,
        0x00, static_cast<uint8_t>(COLUMN_START + PANEL_WIDTH - 1),
    };
    const uint8_t rows[] = {
        0x00, ROW_START,
        0x00, static_cast<uint8_t>(ROW_START + PANEL_HEIGHT - 1),
    };
    writeCommand(0x2A, columns, sizeof(columns));
    writeCommand(0x2B, rows, sizeof(rows));
    writeCommand(0x2C);
}

#ifndef _GPGFX_ST7735_H_
#define _GPGFX_ST7735_H_

#include "interfaces/i2c/ssd1306/tiny_ssd1306.h"

class GPGFX_ST7735 : public GPGFX_TinySSD1306 {
public:
    void init(GPGFX_DisplayTypeOptions options) override;
    void setPower(bool isPowered) override;
    void clear() override;
    uint32_t getPixel(uint8_t x, uint8_t y) override;
    void drawPixel(uint8_t x, uint8_t y, uint32_t color) override;
    void drawBuffer(uint8_t* pBuffer) override;

    bool isSPI() const override { return true; }
    bool isI2C() const override { return false; }

private:
    static constexpr uint16_t LOGICAL_WIDTH = 128;
    static constexpr uint16_t LOGICAL_HEIGHT = 64;
    static constexpr uint16_t PANEL_WIDTH = 160;
    static constexpr uint16_t PANEL_HEIGHT = 80;
    static constexpr uint8_t COLUMN_START = 1;
    static constexpr uint8_t ROW_START = 26;

    GPGFX_DisplayTypeOptions _displayOptions;
    uint16_t frameBuffer[LOGICAL_WIDTH * LOGICAL_HEIGHT]{};
    bool _isPowered = false;

    void writeCommand(uint8_t command, const uint8_t* data = nullptr, size_t length = 0);
    void setAddressWindow();
};

#endif

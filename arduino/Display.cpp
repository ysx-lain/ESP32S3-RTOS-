/**
 * @file Display.cpp
 * @brief ST7735 屏幕驱动模块实现
 * 
 * 实现 Display 类的所有方法, 包括初始化, 绘图, 传感器显示, 
 * 背光控制和硬件检测. 
 * 
 * @author ysx
 * @date 2024-03-02
 * @version 2.6
 */

#include "Display.h"

// 构造函数:初始化 Ucglib 对象(硬件 SPI), 保存背光引脚, 设置内部标志
// 硬件 SPI: 参数顺序 cd (a0), cs, reset
Display::Display(uint8_t sclk, uint8_t data, uint8_t cd, uint8_t cs, uint8_t reset, uint8_t blPin)
    : ucg(cd, cs, reset), 
      screenOn(false), 
      lastActivityTime(0), 
      _initialized(false),
      _blPin(blPin) {
    // 硬件 SPI 使用 ESP32 硬件 SPI 控制器，CLK and MOSI 固定:
    // CLK = GPIO18, MOSI = GPIO23 （ESP32 默认硬件 SPI引脚）
    // 如果你的引脚不同，请修改引脚配置
}

bool Display::begin(uint8_t rotation) {
    ucg.begin(UCG_FONT_MODE_TRANSPARENT);
    if (rotation == 1) ucg.setRotate90();
    else if (rotation == 2) ucg.setRotate180();
    else if (rotation == 3) ucg.setRotate270();
    ucg.clearScreen();
    ucg_ptr = ucg.getUcg();

    // 使用成员变量 _blPin 控制背光
    pinMode(_blPin, OUTPUT);
    digitalWrite(_blPin, HIGH);  // 点亮背光

    lastActivityTime = millis();
    screenOn = true;
    _initialized = true;
    return true;
}

// 硬件自检:尝试清屏并画点, 若 _initialized 为 false 则直接失败
bool Display::checkHardware() {
    if (!_initialized) return false;
    // 简单测试:清屏并画一个白点, 等待短暂时间后恢复
    ucg_ClearScreen(ucg_ptr);
    ucg_SetColor(ucg_ptr, 0, 255, 255, 255);
    ucg_DrawPixel(ucg_ptr, 10, 10);
    delay(10);  // 短暂延时让操作完成(实际不等待显示, 仅示意)
    // 由于无法读取屏幕状态, 我们只能假定如果初始化成功则硬件正常
    return true;
}

void Display::clear() { ucg_ClearScreen(ucg_ptr); }

int Display::width() { return ucg_GetWidth(ucg_ptr); }

int Display::height() { return ucg_GetHeight(ucg_ptr); }

const ucg_fntpgm_uint8_t* Display::getFont(FontSize size) {
    switch (size) {
        case FONT_SMALL_SIZE: return ucg_font_5x7_tr;
        case FONT_MEDIUM_SIZE: return ucg_font_7x13_tr;
        case FONT_LARGE_SIZE: return ucg_font_ncenR18_tr;
        default: return ucg_font_5x7_tr;
    }
}

void Display::formatSensor(char* buffer, size_t len, const char* name, double value, const char* unit) {
    if (unit && unit[0] != '\0') {
        snprintf(buffer, len, "%s: %.2f %s", name, value, unit);
    } else {
        snprintf(buffer, len, "%s: %.2f", name, value);
    }
}

void Display::formatSensor(char* buffer, size_t len, const char* name, int value, const char* unit) {
    if (unit && unit[0] != '\0') {
        snprintf(buffer, len, "%s: %d %s", name, value, unit);
    } else {
        snprintf(buffer, len, "%s: %d", name, value);
    }
}

void Display::drawText(int x, int y, const char* text, FontSize size, uint8_t r, uint8_t g, uint8_t b) {
    const ucg_fntpgm_uint8_t* font = getFont(size);
    ucg_SetFont(ucg_ptr, font);
    ucg_SetColor(ucg_ptr, 0, r, g, b);
    ucg_DrawString(ucg_ptr, x, y, 0, text);
}

void Display::drawTextAt(uint8_t row, uint8_t col, const char* text, FontSize size, uint8_t r, uint8_t g, uint8_t b) {
    const ucg_fntpgm_uint8_t* font = getFont(size);
    int x = 2 + col * FONT_WIDTH(font);
    int y = 2 + row * FONT_HEIGHT(font);
    drawText(x, y, text, size, r, g, b);
}

void Display::drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    ucg_SetColor(ucg_ptr, 0, r, g, b);
    ucg_DrawFrame(ucg_ptr, x, y, w, h);
}

void Display::fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    ucg_SetColor(ucg_ptr, 0, r, g, b);
    ucg_DrawBox(ucg_ptr, x, y, w, h);
}

void Display::drawCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b) {
    ucg_SetColor(ucg_ptr, 0, r, g, b);
    ucg_DrawCircle(ucg_ptr, x, y, rad, UCG_DRAW_ALL);
}

void Display::fillCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b) {
    ucg_SetColor(ucg_ptr, 0, r, g, b);
    ucg_DrawDisc(ucg_ptr, x, y, rad, UCG_DRAW_ALL);
}

void Display::drawLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b) {
    ucg_SetColor(ucg_ptr, 0, r, g, b);
    ucg_DrawLine(ucg_ptr, x1, y1, x2, y2);
}

void Display::bootAnimation() {
    clear();
    int w = width(), h = height();
    const char* loading = "Loading...";
    int textWidth = 7 * 9;
    int textX = (w - textWidth) / 2;
    int textY = h/2 - 15;
    ucg_SetFont(ucg_ptr, ucg_font_7x13_tr);
    ucg_SetColor(ucg_ptr, 0, 255, 255, 255);
    ucg_DrawString(ucg_ptr, textX, textY, 0, loading);
    int barX = 20, barY = h/2 + 5, barW = w - 40, barH = 8;
    drawRect(barX, barY, barW, barH, 128, 128, 128);
    for (int i = 0; i <= barW; i += 2) {
        fillRect(barX, barY, i, barH, 0, 255, 0);
        delay(20);
    }
    delay(300);
}

void Display::sleep() {
    if (!screenOn) return;
    digitalWrite(_blPin, LOW);
    screenOn = false;
}

void Display::wake() {
    if (screenOn) return;
    digitalWrite(_blPin, HIGH);
    delay(5);
    screenOn = true;
}

bool Display::isScreenOn() { return screenOn; }

void Display::updateActivity() {
    lastActivityTime = millis();
    if (!screenOn) {
        wake();
    }
}

void Display::checkSleep(unsigned long timeout) {
    if (screenOn && (millis() - lastActivityTime >= timeout)) {
        sleep();
    }
}

void Display::drawSensor(int x, int y, const char* name, double value, const char* unit,
                               FontSize size, uint8_t r, uint8_t g, uint8_t b) {
    char buffer[64];
    formatSensor(buffer, sizeof(buffer), name, value, unit);
    drawText(x, y, buffer, size, r, g, b);
}

void Display::drawSensor(int x, int y, const char* name, int value, const char* unit,
                               FontSize size, uint8_t r, uint8_t g, uint8_t b) {
    char buffer[64];
    formatSensor(buffer, sizeof(buffer), name, value, unit);
    drawText(x, y, buffer, size, r, g, b);
}

void Display::printSensorAt(uint8_t row, uint8_t col, const char* name, double value, const char* unit,
                                  FontSize size, uint8_t r, uint8_t g, uint8_t b) {
    char buffer[64];
    formatSensor(buffer, sizeof(buffer), name, value, unit);
    drawTextAt(row, col, buffer, size, r, g, b);
}

void Display::printSensorAt(uint8_t row, uint8_t col, const char* name, int value, const char* unit,
                                  FontSize size, uint8_t r, uint8_t g, uint8_t b) {
    char buffer[64];
    formatSensor(buffer, sizeof(buffer), name, value, unit);
    drawTextAt(row, col, buffer, size, r, g, b);
}

void Display::Sensor(uint8_t row, const char* name, double value, const char* unit) {
    printSensorAt(row, 0, name, value, unit, FONT_MEDIUM_SIZE, 255, 255, 255);
}

void Display::Sensor(uint8_t row, const char* name, int value, const char* unit) {
    printSensorAt(row, 0, name, value, unit, FONT_MEDIUM_SIZE, 255, 255, 255);
}

void Display::printSensors(uint8_t startRow, const SensorData sensors[5],
                                 FontSize size, uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < 5; i++) {
        printSensorAt(startRow + i, 0, sensors[i].name, sensors[i].value, sensors[i].unit, size, r, g, b);
    }
}
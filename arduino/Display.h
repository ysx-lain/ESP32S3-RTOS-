/**
 * @file Display.h
 * @brief ST7735 屏幕显示驱动模块(基于 Ucglib)
 * 
 * 封装 Ucglib 库的基本绘图操作, 提供传感器数据显示, 背光控制, 
 * 自动息屏等功能. 针对 80x160 分辨率并旋转 90° 为 160x80 横屏优化. 
 * 
 * @author ysx
 * @date 2024-03-02
 * @version 2.6
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <SPI.h>
#include "Ucglib.h"

// 预定义字体及其尺寸(用于行列布局)
#define FONT_SMALL   ucg_font_5x7_tr
#define FONT_MEDIUM  ucg_font_7x13_tr
#define FONT_LARGE   ucg_font_ncenR18_tr

#define FONT_SMALL_W  5
#define FONT_SMALL_H  7
#define FONT_MEDIUM_W 7
#define FONT_MEDIUM_H 13
#define FONT_LARGE_W  12
#define FONT_LARGE_H  18

// 根据字体指针获取字符宽度/高度(用于行列定位)
#define FONT_WIDTH(font) ( \
    (font) == ucg_font_5x7_tr   ? FONT_SMALL_W : \
    (font) == ucg_font_7x13_tr  ? FONT_MEDIUM_W : \
    FONT_LARGE_W )

#define FONT_HEIGHT(font) ( \
    (font) == ucg_font_5x7_tr   ? FONT_SMALL_H : \
    (font) == ucg_font_7x13_tr  ? FONT_MEDIUM_H : \
    FONT_LARGE_H )

/// 字体大小枚举, 用于函数参数
enum FontSize {
    FONT_SMALL_SIZE,
    FONT_MEDIUM_SIZE,
    FONT_LARGE_SIZE
};

/// 传感器数据结构, 用于批量显示
struct SensorData {
    const char* name;   ///< 传感器名称
    double value;       ///< 数值(支持浮点)
    const char* unit;   ///< 单位, 可为空
};

/**
 * @class Display
 * @brief ST7735 屏幕驱动类, 封装常用绘图及传感器显示功能. 
 * 
 * 提供基于像素坐标和行列的文本绘制, 几何图形绘制, 传感器数据显示, 
 * 开机动画, 背光控制及自动息屏功能. 
 */
class Display {
private:
    Ucglib_ST7735_18x128x160_HWSPI ucg; ///< Ucglib 对象(硬件 SPI) - 更快刷新
    ucg_t *ucg_ptr;                      ///< Ucglib 内部指针, 用于底层调用
    bool screenOn;                       ///< 背光状态:true=点亮
    unsigned long lastActivityTime;      ///< 上次活动时间(用于自动息屏)
    bool _initialized;                   ///< 硬件初始化成功标志(状态检测用)
    uint8_t _blPin;                       ///< 背光控制引脚

    /**
     * @brief 根据字体枚举获取 Ucglib 字体指针
     */
    const ucg_fntpgm_uint8_t* getFont(FontSize size);

    /**
     * @brief 格式化传感器字符串(浮点版)
     */
    void formatSensor(char* buffer, size_t len, const char* name, double value, const char* unit);

    /**
     * @brief 格式化传感器字符串(整数版)
     */
    void formatSensor(char* buffer, size_t len, const char* name, int value, const char* unit);

public:
    /**
     * @brief 构造函数 (硬件 SPI)
     * @param cd    命令/数据引脚(DC/A0)
     * @param cs    片选引脚
     * @param reset 复位引脚
     * @param blPin 背光控制引脚
     * @note ESP32 硬件 SPI 固定引脚: SCLK=GPIO18, MOSI=GPIO23
     */
    Display(uint8_t cd, uint8_t cs, uint8_t reset, uint8_t blPin);

    /**
     * @brief 初始化屏幕, 设置旋转并点亮背光
     * @param rotation 旋转角度:0=0°, 1=90°, 2=180°, 3=270°(默认 90° 横屏)
     * @return true  初始化成功
     * @return false 初始化失败(Ucglib 未提供错误码, 通常总是返回 true)
     */
    bool begin(uint8_t rotation = 1);

    /// @name 硬件状态检测
    ///@{
    /**
     * @brief 检查屏幕是否已成功初始化
     * @return true  已初始化
     * @return false 未初始化或初始化失败
     */
    bool isInitialized() const { return _initialized; }

    /**
     * @brief 硬件自检(尝试清屏并画点)
     * @return true  硬件工作正常(根据简单测试)
     * @note 本函数执行快速清屏和画点测试, 若屏幕未初始化则返回 false. 
     */
    bool checkHardware();
    ///@}

    /// @name 屏幕基本控制
    ///@{
    void clear();               ///< 清屏
    int width();                ///< 获取屏幕宽度
    int height();               ///< 获取屏幕高度
    ///@}

    /// @name 文本绘制(像素坐标)
    ///@{
    void drawText(int x, int y, const char* text, FontSize size, uint8_t r, uint8_t g, uint8_t b);
    void drawTextAt(uint8_t row, uint8_t col, const char* text, FontSize size, uint8_t r, uint8_t g, uint8_t b);
    ///@}

    /// @name 图形绘制
    ///@{
    void drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
    void fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
    void drawCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b);
    void fillCircle(int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b);
    void drawLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
    ///@}

    /// @name 开机动画
    ///@{
    void bootAnimation();       ///< 播放开机动画(Loading 条)
    ///@}

    /// @name 背光控制
    ///@{
    void sleep();               ///< 关闭背光(息屏)
    void wake();                ///< 打开背光(唤醒)
    bool isScreenOn();          ///< 返回当前背光状态
    void updateActivity();      ///< 更新活动时间(通常由按键调用)
    void checkSleep(unsigned long timeout = 10000); ///< 检查超时并自动息屏
    ///@}

    /// @name 传感器数据显示(多种重载)
    ///@{
    void drawSensor(int x, int y, const char* name, double value, const char* unit,
                    FontSize size = FONT_MEDIUM_SIZE, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
    void drawSensor(int x, int y, const char* name, int value, const char* unit,
                    FontSize size = FONT_MEDIUM_SIZE, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
    void printSensorAt(uint8_t row, uint8_t col, const char* name, double value, const char* unit,
                       FontSize size = FONT_MEDIUM_SIZE, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
    void printSensorAt(uint8_t row, uint8_t col, const char* name, int value, const char* unit,
                       FontSize size = FONT_MEDIUM_SIZE, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
    void Sensor(uint8_t row, const char* name, double value, const char* unit);
    void Sensor(uint8_t row, const char* name, int value, const char* unit);
    void printSensors(uint8_t startRow, const SensorData sensors[5],
                      FontSize size = FONT_MEDIUM_SIZE,
                      uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
    ///@}
};

#endif
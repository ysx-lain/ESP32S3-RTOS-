/**
 * @file sketch_feb26a.ino
 * @brief 主程序:FreeRTOS 多任务版本 (硬件 SPI)
 * 
 * 硬件接线(ESP32S3 硬件 SPI):
 *   - 屏幕 硬件 SPI 默认引脚:
 *     SCLK = GPIO18 (硬件 SPI 固定，不能改)
 *     MOSI  = GPIO23 (硬件 SPI 固定，不能改)
 *   - 屏幕 自定义引脚:
 *     dc/a0 = GPIO9
 *     cs    = GPIO10
 *     reset = GPIO8
 *   - 背光:GPIO12(高电平点亮)
 *   - 按键:GPIO2(接 GND, 内部上拉)
 * 
 * 功能:
 *   - FreeRTOS 多任务架构, 每个功能模块一个独立任务
 *   - 按键扫描任务 → 事件队列 → 显示任务
 *   - 传感器读取任务 → 数据队列 → 显示任务 + BLE 任务
 *   - 时间更新任务 → 每分钟刷新显示
 *   - 10秒无操作自动息屏, 按键唤醒
 *   - 可选 BLE 通信
 * 
 * 任务划分:
 *   1. button_task  - 按键扫描, 优先级 1
 *   2. clock_task   - 时间更新, 优先级 1
 *   3. sensor_task  - 传感器读取, 优先级 2
 *   4. display_task - 显示更新, 优先级 2
 *   5. ble_task     - BLE 通信, 优先级 3 (可选)
 * 
 * 页面分配(按键切换):
 *   页面1 - 气体传感器数据(7路)
 *   页面2 - ESP32 内存和工作状态(剩余堆内存等)
 *   页面3 - 时钟/运行时间
 *
 * @author ysx
 * @date 2024-03-16
 * @version 4.1 (FreeRTOS 版本)
 */

#include <SPI.h>
#include <Ucglib.h>
#include "Display.h"
#include "clock.h"
#include "rtos_config.h"
#include "ble.h"      // BLE 模块已启用

// ==================== 硬件引脚定义 ====================
// 硬件 SPI 使用 ESP32S3 默认硬件 SPI 引脚，不可修改:
//  SCLK = GPIO18
//  MOSI = GPIO23
#define BUTTON_PIN 2
#define BL_PIN     12
#define DC_PIN     9    // 命令/数据
#define CS_PIN     10   // 片选
#define RESET_PIN  8    // 复位

// ==================== 全局对象 ====================
// 硬件 SPI 构造函数: Display(dc, cs, reset, blPin)
Display display(DC_PIN, CS_PIN, RESET_PIN, BL_PIN);

// ==================== 页面管理 ====================
const int PAGE_COUNT = 3;
int currentPage = 0;
unsigned long lastButtonPress = 0;
// debounceTime 已在 rtos_config.cpp 中定义

typedef void (*PageFunction)();
void page1();  // 传感器页
void page2();  // 运行时间页
void page3();  // 系统状态页
PageFunction pages[PAGE_COUNT] = { page1, page2, page3 };

// ==================== 时间显示相关(优化版)====================
// 强制刷新运行时间显示(用于页面切换时立即显示最新时间)
void refreshTimeDisplay() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Uptime: %lud %02lu:%02lu", days, hours % 24, minutes % 60);

    // 中号字体(7x13)的高度为13像素, 基线y=50
    const int fontHeight = 13;
    const int baselineY = 50;
    const int clearX = 10;
    const int clearY = baselineY - fontHeight;   // 文本顶部坐标
    const int clearW = 150;                       // 足够宽覆盖整个字符串
    const int clearH = fontHeight + 2;             // 多清除一点以防下降部分残留

    display.fillRect(clearX, clearY, clearW, clearH, 0, 0, 0);
    display.drawText(10, baselineY, buffer, FONT_MEDIUM_SIZE, 255, 255, 255);
}

// ==================== 外部全局变量 ====================
extern SensorReading_t latestSensorReading;

// ======================================================
// 屏幕尺寸: 宽度 160px × 高度 80px (横向)
// 字体: FONT_MEDIUM = 7px宽 × 13px高
// 行间距 = 13px + 2px = 15px
// 左边标签宽度 = 45px，右边数值区域 = 110px
// ======================================================
#define ROW_HEIGHT   15    // 每行总高度
#define LABEL_WIDTH  45    // 标签区域宽度
#define VALUE_X      50    // 数值区域起始X

// 页面1首次绘制：绘制所有标签+初始数值
// Y坐标从 0 开始计算，完美放7行正好 7×15 = 105 → 80 不够，压缩行间距到 12px
#undef ROW_HEIGHT
#define ROW_HEIGHT   12    // 压缩行间距，7行正好 7×12=84，留一点边距正好放下

const int row_y[7] = { 2, 14, 26, 38, 50, 62, 74 };  // 每行起始Y坐标
const int clear_h = ROW_HEIGHT - 1;                   // 清除区域高度

// 页面1首次绘制：绘制所有标签+初始数值
void page1_init() {
    // 第一列绘制标签，数值位置留空，后续只更新数值
    display.drawText( 5, row_y[0]+10,  "Temp:",  FONT_MEDIUM_SIZE, 255, 255, 255);
    display.drawText( 5, row_y[1]+10,  "Hum:",   FONT_MEDIUM_SIZE, 255, 255, 255);
    display.drawText( 5, row_y[2]+10,  "Press:", FONT_MEDIUM_SIZE, 255, 255, 255);
    display.drawText( 5, row_y[3]+10,  "CO₂:",   FONT_MEDIUM_SIZE, 255, 255, 255);
    display.drawText( 5, row_y[4]+10,  "O₃:",   FONT_MEDIUM_SIZE, 255, 255, 255);
    display.drawText( 5, row_y[5]+10,  "C₂H₄O:", FONT_MEDIUM_SIZE, 255, 255, 255);
    display.drawText( 5, row_y[6]+10,  "C₂H₄:",  FONT_MEDIUM_SIZE, 255, 255, 255);
}

// 页面1增量更新：只更新数值，不重绘标签，更快
void page1_update() {
    SensorReading_t reading;
    
    // 读取最新数据（需要互斥锁保护）
    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(100))) {
        reading = latestSensorReading;
        xSemaphoreGive(xSensorDataMutex);
    }
    
    // 只清除数值区域，重绘最新数值
    // 110px宽度足够放下 "xxx.x ℃"，完全放下
    display.fillRect(VALUE_X, row_y[0], 105, clear_h, 0, 0, 0);
    display.drawSensor(VALUE_X, row_y[0]+10, "Temp",  reading.temperature,  "C");
    
    display.fillRect(VALUE_X, row_y[1], 105, clear_h, 0, 0, 0);
    display.drawSensor(VALUE_X, row_y[1]+10, "Hum",   reading.humidity,    "%");
    
    display.fillRect(VALUE_X, row_y[2], 105, clear_h, 0, 0, 0);
    display.drawSensor(VALUE_X, row_y[2]+10, "Press", reading.pressure,  "hPa");
    
    display.fillRect(VALUE_X, row_y[3], 105, clear_h, 0, 0, 0);
    display.drawSensor(VALUE_X, row_y[3]+10, "CO₂",   reading.co2,        "ppm");
    
    display.fillRect(VALUE_X, row_y[4], 105, clear_h, 0, 0, 0);
    display.drawSensor(VALUE_X, row_y[4]+10, "O₃",   reading.ozone,      "ppb");
    
    display.fillRect(VALUE_X, row_y[5], 105, clear_h, 0, 0, 0);
    display.drawSensor(VALUE_X, row_y[5]+10, "C₂H₄O", reading.acetaldehyde, "ppb");
    
    display.fillRect(VALUE_X, row_y[6], 105, clear_h, 0, 0, 0);
    display.drawSensor(VALUE_X, row_y[6]+10, "C₂H₄",  reading.ethylene,   "ppm");
}

// ==================== 页面函数 ====================
void page1() {
    page1_init();      // 首次打开页面：绘制标签
    page1_update();   // 绘制初始数值
}

void page2() {
    // 页面2: ESP32 内存和工作状态
    display.drawText(15, 15, "System Info", FONT_LARGE_SIZE, 0, 255, 255);
    
    // 获取堆内存信息
    size_t freeHeap = xPortGetFreeHeapSize();
    size_t minimumFreeHeap = xPortGetMinimumEverFreeHeapSize();
    int freeKB = freeHeap / 1024;
    int minFreeKB = minimumFreeHeap / 1024;
    
    // 获取当前运行核心
    int coreId = xPortGetCoreID();
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Free: %d KB", freeKB);
    display.drawText(10, 35, buffer, FONT_MEDIUM_SIZE, 255, 255, 255);
    
    snprintf(buffer, sizeof(buffer), "MinFree: %d KB", minFreeKB);
    display.drawText(10, 50, buffer, FONT_MEDIUM_SIZE, 255, 255, 255);
    
    snprintf(buffer, sizeof(buffer), "Running Core: %d", coreId);
    display.drawText(10, 65, buffer, FONT_MEDIUM_SIZE, 255, 255, 255);
}

void page3() {
    // 页面3: 时钟/运行时间
    display.drawText(20, 20, "Uptime", FONT_LARGE_SIZE, 0, 255, 255);
    refreshTimeDisplay();   // 立即显示当前时间(内部已处理局部清除)
}

// ==================== BLE 回调(可选)====================
// 如需启用 BLE, 取消注释以下部分, 并在 setup() 中调用 BLEManager::onReceiveData(onBLECommand);
/*
void onBLECommand(const String& cmd) {
    Serial.print("BLE command: ");
    Serial.println(cmd);
}
*/

// ==================== 初始化 ====================
void setup() 
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== ESP32-S3 RTOS SmartWatch starting ===");

    // 硬件引脚初始化
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // 初始化屏幕
    if (!display.begin(1)) {
        Serial.println("Display init failed!");
        while (1);
    }
    Serial.println("Display ready");

    // 初始化系统时钟(可选, 暂不使用其 ticks)
    // SystemClock::begin();

    // 初始化 BLE(已启用)
    BLEManager::begin("ESP32-S3_Sensor");
    // BLEManager::onReceiveData(onBLECommand);

    // 初始化 FreeRTOS:创建互斥锁、队列、所有任务
    rtos_init();

    Serial.println("=== System ready (FreeRTOS multi-task mode ===");
}

// ==================== 主循环 ====================
// 在 Arduino-ESP32 中, loop() 本身也是一个 FreeRTOS 任务
// 这里我们不需要做任何事, 所有工作都在各个任务中完成
void loop() {
    // 无事可做, 睡眠让出CPU
    vTaskDelay(pdMS_TO_TICKS(1000));
}
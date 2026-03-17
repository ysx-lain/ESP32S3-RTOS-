/**
 * @file ESP32S3-RTOS.ino
 * @brief 主程序:FreeRTOS 多任务版本
 * 
 * 硬件接线:
 *   - 屏幕:sclk=13, data=11, cd=9, cs=10, reset=8
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
 * @author ysx
 * @date 2024-03-16
 * @version 4.0 (FreeRTOS 版本)
 */

#include <SPI.h>
#include <Ucglib.h>
#include "Display.h"
#include "clock.h"
#include "rtos_config.h"
#include "ble.h"      // BLE 模块已启用

// ==================== 硬件引脚定义 ====================
#define BUTTON_PIN 2
#define BL_PIN     12

// ==================== 全局对象 ====================
Display display(13, 11, 9, 10, 8, BL_PIN);

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

// ==================== 页面函数 ====================
void page1() {
    // 从传感器队列获取最新数据显示
    SensorReading_t reading;
    // 非阻塞读取最新数据, 持续读取直到队列为空, 保留最后一个(最新的)
    while (xQueueReceive(xSensorDataQueue, &reading, 0)) {
        // 循环读取, 最后一个就是最新数据
    }
    
    // 显示最新读取的传感器数据
    display.Sensor(1, "Temp", reading.temperature, "C");
    display.Sensor(2, "Hum",  reading.humidity,   "%");
    display.Sensor(3, "Press", reading.pressure, "hPa");
    display.Sensor(4, "CO₂",   reading.co2,  "ppm");
    display.Sensor(5, "O₃",   reading.ozone,  "ppb");
    display.Sensor(6, "C₂H₄O", reading.acetaldehyde, "ppb");
    display.Sensor(7, "C₂H₄", reading.ethylene, "ppm");
}

void page2() {
    // 运行时间页面:先绘制标题, 再显示当前时间
    display.drawText(20, 20, "Uptime", FONT_LARGE_SIZE, 0, 255, 255);
    refreshTimeDisplay();   // 立即显示当前时间(内部已处理局部清除)
}

// ==================== 系统状态页面:显示 BLE 连接状态和系统信息
void page3() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    bool bleConnected = BLEManager::isConnected();
    
    display.drawText(20, 15, "System Status", FONT_MEDIUM_SIZE, 255, 255, 255);
    
    if (bleConnected) {
        display.drawText(10, 35, "BLE: Connected", FONT_MEDIUM_SIZE, 0, 255, 0);
    } else {
        display.drawText(10, 35, "BLE: Disconnected", FONT_MEDIUM_SIZE, 255, 0, 0);
    }
    
    char uptime[32];
    snprintf(uptime, sizeof(uptime), "Uptime: %luh%lum", hours, minutes % 60);
    display.drawText(10, 50, uptime, FONT_MEDIUM_SIZE, 255, 255, 255);
    
    char pageInfo[16];
    snprintf(pageInfo, sizeof(pageInfo), "Page: %d/%d", currentPage + 1, PAGE_COUNT);
    display.drawText(10, 65, pageInfo, FONT_MEDIUM_SIZE, 255, 255, 255);
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
void setup() {
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

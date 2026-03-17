/**
 * @file ESP32S3-RTOS.ino
 * @brief ESP32S3 FreeRTOS 多任务 BLE 传感器采集项目 - 主入口
 *
 * 项目架构：
 * 按功能拆分多文件，每个任务一个独立文件，结构清晰易维护：
 * ├── ESP32S3-RTOS.ino  (本文件，仅初始化)
 * ├── Config.h          (全局配置:引脚、栈大小、优先级、范围、日志宏)
 * ├── TaskInit.h/cpp    (所有任务初始化:创建互斥锁、队列、任务)
 * ├── ButtonTask.h/cpp  (按键扫描任务，消抖，事件队列)
 * ├── ClockTask.h/cpp   (时钟/运行时间任务)
 * ├── SensorTask.h/cpp  (传感器采集任务，模拟数据生成)
 * ├── DisplayTask.h/cpp (显示更新任务，页面管理，自动息屏)
 * ├── BLETask.h/cpp     (BLE 通信任务，二进制数据发送)
 * ├── ble.h/cpp         (BLE 模块封装，基于 NimBLE)
 * ├── Display.h/cpp     (屏幕驱动封装，基于 Ucglib)
 * └── web-serial-client/(网页串口客户端，无需编译)
 *
 * 规范：
 * - 所有延时统一使用 vTaskDelay(pdMS_TO_TICKS(ms))
 * - 不使用 Arduino delay()，避免阻塞 CPU
 * - 共享资源使用互斥锁保护
 * - 统一栈大小和优先级配置
 * - 完整日志输出，方便调试
 *
 * 作者：ysx
 * 重构：OpenClaw
 * 日期：2026-03-17
 * 版本：5.0（架构重构版）
 */

// 项目头文件
#include "Config.h"
#include "TaskInit.h"
#include "Display.h"
#include "ble.h"
#include <Arduino.h>
#include <Ucglib.h>

// 全局显示对象
Display display(SCREEN_SCLK, SCREEN_MOSI, SCREEN_CD, SCREEN_CS, SCREEN_RESET, BL_PIN);

void setup() {
    // 初始化串口
    Serial.begin(115200);
    delay(1000); // 给串口一点时间稳定
    LOG_INFO("=== ESP32S3 FreeRTOS BLE Sensor Project starting ===");

    // 硬件引脚初始化
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    LOG_INFO("Hardware pins initialized");

    // 初始化屏幕
    if (!display.begin(1)) {
        LOG_ERROR("Display initialization failed! Halting.");
        while (1);
    }
    LOG_INFO("Display ready");

    // 初始化 BLE
    BLEManager::begin(BLE_DEVICE_NAME);
    LOG_INFO("BLE initialized, device name: %s", BLE_DEVICE_NAME);

    // 初始化所有 FreeRTOS 任务
    rtos_init_all();

    LOG_INFO("=== System ready, all tasks started ===");
}

void loop() {
    // 所有工作都在 FreeRTOS 任务中完成
    // loop() 仅用于 Arduino 兼容，这里睡眠让出 CPU
    vTaskDelay(pdMS_TO_TICKS(1000));
}

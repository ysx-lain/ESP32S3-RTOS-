/**
 * @file TaskInit.cpp
 * @brief 所有 FreeRTOS 任务初始化实现
 *
 * 功能：
 * - 创建互斥锁保护共享资源
 * - 创建任务间通信队列
 * - 创建所有任务，固定到 Core 1 运行（Arduino 核心在 Core 0）
 * - 统一栈大小和优先级配置
 */

#include "TaskInit.h"
#include "Config.h"
#include "ButtonTask.h"
#include "ClockTask.h"
#include "SensorTask.h"
#include "DisplayTask.h"
#include "BLETask.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// 互斥锁定义
SemaphoreHandle_t xDisplayMutex = nullptr;
SemaphoreHandle_t xSensorDataMutex = nullptr;

// 任务句柄（用于调试，如果不需要可以删除）
static TaskHandle_t xButtonTaskHandle = nullptr;
static TaskHandle_t xClockTaskHandle = nullptr;
static TaskHandle_t xSensorTaskHandle = nullptr;
static TaskHandle_t xDisplayTaskHandle = nullptr;
static TaskHandle_t xBLETaskHandle = nullptr;

/**
 * @brief 初始化所有 RTOS 资源和任务
 */
void rtos_init_all(void) {
    // 1. 创建互斥锁
    xDisplayMutex = xSemaphoreCreateMutex();
    if (xDisplayMutex == nullptr) {
        LOG_ERROR("Failed to create display mutex");
        return;
    }

    xSensorDataMutex = xSemaphoreCreateMutex();
    if (xSensorDataMutex == nullptr) {
        LOG_ERROR("Failed to create sensor data mutex");
        return;
    }

    // 2. 创建队列
    xButtonEventQueue = xQueueCreate(QUEUE_LENGTH_BUTTON, sizeof(ButtonEvent_t));
    if (xButtonEventQueue == nullptr) {
        LOG_ERROR("Failed to create button event queue");
        return;
    }

    xSensorDataQueue = xQueueCreate(QUEUE_LENGTH_SENSOR, sizeof(SensorReading_t));
    if (xSensorDataQueue == nullptr) {
        LOG_ERROR("Failed to create sensor data queue");
        return;
    }

    // 3. 创建所有任务，固定到 Core 1 运行
    // Arduino 核心在 Core 0 运行，应用任务放 Core 1 避免阻塞

    // 按键扫描任务
    xTaskCreatePinnedToCore(
        button_task,
        "Button",
        TASK_STACK_BUTTON,
        nullptr,
        TASK_PRIORITY_BUTTON,
        &xButtonTaskHandle,
        1
    );
    LOG_INFO("Created button task on core 1, stack=%d priority=%d",
             TASK_STACK_BUTTON, TASK_PRIORITY_BUTTON);

    // 时钟任务
    xTaskCreatePinnedToCore(
        clock_task,
        "Clock",
        TASK_STACK_CLOCK,
        nullptr,
        TASK_PRIORITY_CLOCK,
        &xClockTaskHandle,
        1
    );
    LOG_INFO("Created clock task on core 1, stack=%d priority=%d",
             TASK_STACK_CLOCK, TASK_PRIORITY_CLOCK);

    // 传感器采集任务
    xTaskCreatePinnedToCore(
        sensor_task,
        "Sensor",
        TASK_STACK_SENSOR,
        nullptr,
        TASK_PRIORITY_SENSOR,
        &xSensorTaskHandle,
        1
    );
    LOG_INFO("Created sensor task on core 1, stack=%d priority=%d",
             TASK_STACK_SENSOR, TASK_PRIORITY_SENSOR);

    // 显示更新任务
    xTaskCreatePinnedToCore(
        display_task,
        "Display",
        TASK_STACK_DISPLAY,
        nullptr,
        TASK_PRIORITY_DISPLAY,
        &xDisplayTaskHandle,
        1
    );
    LOG_INFO("Created display task on core 1, stack=%d priority=%d",
             TASK_STACK_DISPLAY, TASK_PRIORITY_DISPLAY);

    // BLE 通信任务
    xTaskCreatePinnedToCore(
        ble_task,
        "BLE",
        TASK_STACK_BLE,
        nullptr,
        TASK_PRIORITY_BLE,
        &xBLETaskHandle,
        1
    );
    LOG_INFO("Created BLE task on core 1, stack=%d priority=%d",
             TASK_STACK_BLE, TASK_PRIORITY_BLE);

    LOG_INFO("All tasks created successfully");
}

/**
 * @file DisplayTask.cpp
 * @brief 显示更新任务实现
 *
 * 功能：
 * - 等待按键事件，处理页面切换、唤醒屏幕
 * - 多页面管理，循环切换
 * - 自动息屏功能，超时关闭背光
 * - 互斥锁保护显示资源，避免多任务并发访问
 */

#include "DisplayTask.h"
#include "Config.h"
#include "SensorTask.h"
#include "ButtonTask.h"
#include "ClockTask.h"
#include "ble.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// 外部声明全局显示对象
extern Display display;

// 页面管理变量
const int PAGE_COUNT = 3;
int currentPage = 0;
unsigned long lastActivityTime = 0;

// 显示互斥锁 - 保护显示资源不被多任务并发访问
SemaphoreHandle_t xDisplayMutex = nullptr;

// 页面函数数组
static PageFunction pages[PAGE_COUNT] = {
    page1_sensor,
    page2_uptime,
    page3_status
};

/**
 * @brief 第一页：传感器数据显示
 */
void page1_sensor() {
    // 读取最新传感器数据
    SensorReading_t reading;
    while (xQueueReceive(xSensorDataQueue, &reading, 0)) {
        // 持续读取，保留最新一个
    }

    // 逐行显示所有传感器
    display.Sensor(1, "Temp", reading.temperature, "C");
    display.Sensor(2, "Hum", reading.humidity, "%");
    display.Sensor(3, "Press", reading.pressure, "hPa");
    display.Sensor(4, "CO₂", reading.co2, "ppm");
    display.Sensor(5, "O₃", reading.ozone, "ppb");
    display.Sensor(6, "C₂H₄O", reading.acetaldehyde, "ppb");
    display.Sensor(7, "C₂H₄", reading.ethylene, "ppm");
}

/**
 * @brief 第二页：运行时间显示
 */
void page2_uptime() {
    display.drawText(20, 20, "Uptime", FONT_LARGE_SIZE, 0, 255, 255);
    refreshTimeDisplay();
}

/**
 * @brief 第三页：系统状态显示
 */
void page3_status() {
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

/**
 * @brief 显示任务入口函数
 */
void display_task(void *pvParameters) {
    // 获取显示互斥锁
    if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY)) {
        // 初始化显示
        display.clear();
        pages[currentPage]();
        lastActivityTime = millis();
        xSemaphoreGive(xDisplayMutex);
    }

    LOG_INFO("Display task started, initial page: %d", currentPage);

    for (;;) {
        ButtonEvent_t event;

        // 阻塞等待按键事件
        if (xQueueReceive(xButtonEventQueue, &event, portMAX_DELAY)) {
            if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(100))) {
                bool wasScreenOff = !display.isScreenOn();
                lastActivityTime = millis();
                display.updateActivity();

                if (wasScreenOff) {
                    // 唤醒屏幕，重绘当前页
                    display.wake();
                    display.clear();
                    pages[currentPage]();
                    LOG_INFO("Screen woken up");
                } else {
                    // 切换到下一页
                    currentPage = (currentPage + 1) % PAGE_COUNT;
                    display.clear();
                    pages[currentPage]();
                    LOG_INFO("Switched to page %d/%d", currentPage + 1, PAGE_COUNT);
                }

                xSemaphoreGive(xDisplayMutex);
            } else {
                LOG_WARN("Failed to get display mutex for button event");
            }
        }

        // 检查自动息屏
        if (display.isScreenOn() && (millis() - lastActivityTime) > SCREEN_TIMEOUT) {
            if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(100))) {
                display.sleep();
                LOG_INFO("Screen auto sleep after timeout");
                xSemaphoreGive(xDisplayMutex);
            }
        }
    }

    vTaskDelete(nullptr);
}

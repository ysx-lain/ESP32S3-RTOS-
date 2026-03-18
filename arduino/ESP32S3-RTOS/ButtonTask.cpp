/**
 * @file ButtonTask.cpp
 * @brief 按键扫描任务实现
 *
 * 功能：
 * - 定时扫描按键状态
 * - 软件消抖
 * - 按下事件发送到事件队列
 */

#include "ButtonTask.h"
#include "Config.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// 按键事件队列定义
QueueHandle_t xButtonEventQueue = nullptr;

// 消抖配置
#define DEBOUNCE_DELAY 50  // 消抖延时 ms

/**
 * @brief 按键扫描任务入口函数
 */
void button_task(void *pvParameters) {
    int lastState = HIGH;
    unsigned long lastDebounceTime = 0;

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    for (;;) {
        int currentState = digitalRead(BUTTON_PIN);

        // 状态变化且经过消抖时间后才认为是有效按键
        if (currentState != lastState) {
            lastDebounceTime = millis();
        }

        if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
            if (currentState == LOW && lastState == HIGH) {
                // 有效按下事件，发送到队列
                ButtonEvent_t event = BUTTON_EVENT_PRESS;
                if (xQueueSend(xButtonEventQueue, &event, 0) != pdTRUE) {
                    LOG_WARN("Button queue full, drop event");
                }
                LOG_INFO("Button pressed");
            }
        }

        lastState = currentState;

        // 定时扫描，让出 CPU
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_BUTTON));
    }

    vTaskDelete(nullptr);
}

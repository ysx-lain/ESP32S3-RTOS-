/**
 * @file ButtonTask.h
 * @brief 按键扫描任务头文件
 */

#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include "Config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// 按键事件类型
typedef enum {
    BUTTON_EVENT_PRESS = 0,    // 按键按下事件
} ButtonEvent_t;

// 按键事件队列
extern QueueHandle_t xButtonEventQueue;

/**
 * @brief 按键扫描任务入口函数
 */
void button_task(void *pvParameters);

#endif // BUTTON_TASK_H

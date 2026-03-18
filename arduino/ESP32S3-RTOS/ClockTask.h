/**
 * @file ClockTask.h
 * @brief 时钟/系统运行时间任务头文件
 */

#ifndef CLOCK_TASK_H
#define CLOCK_TASK_H

#include "Config.h"
#include <freertos/FreeRTOS.h>

/**
 * @brief 时钟任务入口函数
 */
void clock_task(void *pvParameters);

/**
 * @brief 强制刷新运行时间显示（页面切换时调用）
 */
void refreshTimeDisplay(void);

#endif // CLOCK_TASK_H

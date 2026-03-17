/**
 * @file DisplayTask.h
 * @brief 显示更新任务头文件
 */

#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "Config.h"
#include "SensorTask.h"
#include "ButtonTask.h"
#include <freertos/FreeRTOS.h>
#include "Display.h"

// 页面总数
extern const int PAGE_COUNT;

// 当前页码（0 起始）
extern int currentPage;

// 页面函数类型
typedef void (*PageFunction)();

/**
 * @brief 显示任务入口函数
 */
void display_task(void *pvParameters);

// 各个页面函数声明
void page1_sensor();   // 传感器数据页
void page2_uptime();   // 运行时间页
void page3_status();  // 系统状态页

#endif // DISPLAY_TASK_H

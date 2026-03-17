/**
 * @file TaskInit.h
 * @brief 所有 FreeRTOS 任务初始化声明
 *
 * 功能：
 * - 创建互斥锁、队列
 * - 创建所有任务
 * - 统一管理任务创建
 */

#ifndef TASK_INIT_H
#define TASK_INIT_H

#include "Config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 显示互斥锁（多任务访问显示资源需要互斥）
extern SemaphoreHandle_t xDisplayMutex;
// 传感器数据互斥锁（可选，用于保护共享传感器数据）
extern SemaphoreHandle_t xSensorDataMutex;

/**
 * @brief 初始化所有任务：创建互斥锁、队列、创建任务
 */
void rtos_init_all(void);

#endif // TASK_INIT_H

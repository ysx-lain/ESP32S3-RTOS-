/**
 * @file BLETask.h
 * @brief BLE 通信任务头文件
 */

#ifndef BLE_TASK_H
#define BLE_TASK_H

#include "Config.h"
#include "SensorTask.h"
#include <freertos/FreeRTOS.h>

/**
 * @brief BLE 任务入口函数
 */
void ble_task(void *pvParameters);

#endif // BLE_TASK_H

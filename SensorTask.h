/**
 * @file SensorTask.h
 * @brief 传感器采集任务头文件
 *
 * 功能：
 * - 定义传感器数据结构
 * - 任务函数声明
 * - 提供传感器数据队列对外接口
 */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "Config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// 传感器数据结构体 - 二进制传输格式
// 所有数据按 4 字节对齐，方便 JavaScript 解析
typedef struct {
    float temperature;          // 温度 ℃
    float humidity;             // 湿度 %RH
    int count;                  // 数据包计数
    float pressure;             // 气压 hPa
    int co2;                    // CO₂ 浓度 ppm
    float ozone;                // 臭氧 O₃ ppb
    float acetaldehyde;         // 乙醛 C₂H₄O ppb
    float ethylene;             // 乙烯 C₂H₄ ppm
    unsigned long timestamp;    // 时间戳 ms
} SensorReading_t;

// 传感器数据队列 - 采集任务输出给显示和 BLE 任务
extern QueueHandle_t xSensorDataQueue;

/**
 * @brief 传感器采集任务入口函数
 * @param pvParameters 任务参数（未使用）
 */
void sensor_task(void *pvParameters);

#endif // SENSOR_TASK_H

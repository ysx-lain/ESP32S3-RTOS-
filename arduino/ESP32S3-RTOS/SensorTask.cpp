/**
 * @file SensorTask.cpp
 * @brief 传感器采集任务实现
 *
 * 功能：
 * - 按照指定间隔采集传感器数据（目前模拟，后续可接入真实传感器）
 * - 使用随机游走算法生成自然变化的模拟数据
 * - 将数据发送到传感器数据队列，供显示任务和BLE任务使用
 */

#include "SensorTask.h"
#include "Config.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// 传感器数据队列定义
QueueHandle_t xSensorDataQueue = nullptr;

/**
 * @brief 钳位数值到指定范围
 * @param value 输入值
 * @param min 最小值
 * @param max 最大值
 * @return 钳位后的值
 */
static inline float clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static inline int clampi(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief 传感器采集任务入口函数
 */
void sensor_task(void *pvParameters) {
    // 初始化基准值
    static float base_temp = 25.0f;
    static float base_hum = 50.0f;
    static float base_press = 1013.0f;
    static int base_co2 = 420;
    static float base_ozone = 30.0f;
    static float base_acetaldehyde = 5.0f;
    static float base_ethylene = 0.5f;
    static int counter = 0;

    // 打印结构体大小信息，便于调试
    LOG_INFO("SensorReading_t size = %zu bytes (expected 36 bytes)", sizeof(SensorReading_t));

    for (;;) {
        SensorReading_t reading;

        // 温度：20-35℃ 随机游走
        base_temp += ((rand() % 100) - 50) / 100.0f;
        base_temp = clampf(base_temp, SENSOR_TEMP_MIN, SENSOR_TEMP_MAX);
        reading.temperature = base_temp;

        // 湿度：30-80%RH 随机游走
        base_hum += ((rand() % 100) - 50) / 10.0f;
        base_hum = clampf(base_hum, SENSOR_HUM_MIN, SENSOR_HUM_MAX);
        reading.humidity = base_hum;

        // 气压：980-1040hPa 随机游走
        base_press += ((rand() % 100) - 50) / 10.0f;
        base_press = clampf(base_press, SENSOR_PRESS_MIN, SENSOR_PRESS_MAX);
        reading.pressure = base_press;

        // CO₂：400-2000ppm 随机游走
        base_co2 += (rand() % 21) - 10;
        base_co2 = clampi(base_co2, SENSOR_CO2_MIN, SENSOR_CO2_MAX);
        reading.co2 = base_co2;

        // 臭氧：0-100ppb 随机游走
        base_ozone += ((rand() % 100) - 50) / 10.0f;
        base_ozone = clampf(base_ozone, SENSOR_OZONE_MIN, SENSOR_OZONE_MAX);
        reading.ozone = base_ozone;

        // 乙醛：0-50ppb 随机游走
        base_acetaldehyde += ((rand() % 100) - 50) / 100.0f;
        base_acetaldehyde = clampf(base_acetaldehyde, SENSOR_ACETAL_MIN, SENSOR_ACETAL_MAX);
        reading.acetaldehyde = base_acetaldehyde;

        // 乙烯：0-5ppm 随机游走
        base_ethylene += ((rand() % 100) - 50) / 1000.0f;
        base_ethylene = clampf(base_ethylene, SENSOR_ETHYLENE_MIN, SENSOR_ETHYLENE_MAX);
        reading.ethylene = base_ethylene;

        // 计数器递增
        counter++;
        reading.count = counter;

        // 时间戳
        reading.timestamp = millis();

        // 发送到队列，超时等待 10ms
        if (xQueueSend(xSensorDataQueue, &reading, pdMS_TO_TICKS(10)) != pdTRUE) {
            LOG_WARN("Sensor queue full, drop packet %d", counter);
        }

        // 使用 FreeRTOS API 延时，让出 CPU
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_SENSOR));
    }

    // 正常不会走到这里
    vTaskDelete(nullptr);
}

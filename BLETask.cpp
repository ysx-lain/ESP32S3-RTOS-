/**
 * @file BLETask.cpp
 * @brief BLE 通信任务实现
 *
 * 功能：
 * - 从传感器数据队列获取最新数据
 * - 通过 BLE 以二进制格式发送给客户端
 * - BLE 断开后自动维护状态
 * - 控制发送频率，避免超过 10Hz
 */

#include "BLETask.h"
#include "Config.h"
#include "SensorTask.h"
#include "ble.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

/**
 * @brief BLE 任务入口函数
 */
void ble_task(void *pvParameters) {
    SensorReading_t lastReading;
    bool hasReceivedData = false;

    LOG_INFO("BLE task started");

    for (;;) {
        if (BLEManager::isConnected()) {
            // 读取最新传感器数据，覆盖旧数据保留最新
            while (xQueueReceive(xSensorDataQueue, &lastReading, 0)) {
                hasReceivedData = true;
            }

            // 如果有数据，发送给 BLE 客户端
            if (hasReceivedData) {
                lastReading.timestamp = millis();
                size_t packetSize = sizeof(lastReading);
                bool sent = BLEManager::sendSensorData((uint8_t*)&lastReading, packetSize);

                if (sent) {
                    LOG_INFO("[BLE-TX] size=%zu bytes temp=%.1f hum=%.1f press=%.1f co2=%d ozone=%.1f acet=%.1f ethylene=%.2f",
                           packetSize,
                           lastReading.temperature,
                           lastReading.humidity,
                           lastReading.pressure,
                           lastReading.co2,
                           lastReading.ozone,
                           lastReading.acetaldehyde,
                           lastReading.ethylene);
                }
            }
        } else {
            // 断开连接时 BLE 栈维护
            BLEManager::update();
            hasReceivedData = false;
        }

        // 定时检查，使用 FreeRTOS 延时
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_BLE));
    }

    vTaskDelete(nullptr);
}

/**
 * @file rtos_config.cpp
 * @brief FreeRTOS 任务实现
 * @details 将各个功能模块划分为独立的 FreeRTOS 任务
 *
 *  任务架构：
 *  - 按键任务：扫描按键，发送事件到队列
 *  - 时钟任务：每分钟更新时间
 *  - 传感器任务：定期读取传感器，发送数据到队列
 *  - 显示任务：接收按键事件和传感器数据，更新显示
 *  - BLE 任务：如果连接，定期发送传感器数据
 *
 * @author ysx
 * @date 2024-03-16
 */

#include "rtos_config.h"
#include "Display.h"
#include "clock.h"
#include "ble.h"

// ==================== 全局变量定义 ====================
TaskHandle_t xButtonTaskHandle = nullptr;
TaskHandle_t xClockTaskHandle = nullptr;
TaskHandle_t xSensorTaskHandle = nullptr;
TaskHandle_t xDisplayTaskHandle = nullptr;
TaskHandle_t xBLETaskHandle = nullptr;

SemaphoreHandle_t xDisplayMutex = nullptr;
SemaphoreHandle_t xSensorDataMutex = nullptr;

QueueHandle_t xButtonEventQueue = nullptr;
QueueHandle_t xSensorDataQueue = nullptr;

// ==================== 外部全局变量声明 ====================
extern Display display;
extern int currentPage;
extern void page1();
extern void page2();
extern void refreshTimeDisplay();
extern unsigned long lastButtonPress;
// 引脚定义已经在 rtos_config.h 中提供

// ==================== 常量定义 ====================
const unsigned long debounceTime = 200;  // 按键消抖时间（毫秒）

// ==================== 按键扫描任务 ====================
void button_task(void *pvParameters) {
    const int buttonPin = BUTTON_PIN;
    pinMode(buttonPin, INPUT_PULLUP);
    int lastButtonState = HIGH;

    for (;;) { // 任务主循环
        int buttonState = digitalRead(buttonPin);

        // 检测按键按下（下降沿）
        if (buttonState == LOW && lastButtonState == HIGH) {
            unsigned long now = xTaskGetTickCount() * portTICK_PERIOD_MS;

            // 消抖检测
            if (now - lastButtonPress > debounceTime) {
                lastButtonPress = now;

                // 发送按键事件到显示任务
                ButtonEvent_t event;
                event.timestamp = now;
                event.isLongPress = false; // 这里只检测短按，可扩展长按

                // 非阻塞发送，如果队列满就丢弃（不等待）
                xQueueSend(xButtonEventQueue, &event, 0);
            }
        }

        lastButtonState = buttonState;

        // 延时，让出CPU
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_BUTTON));
    }

    // 不会走到这里，如果退出要删除自己
    vTaskDelete(nullptr);
}

// ==================== 时间更新任务 ====================
void clock_task(void *pvParameters) {
    static int lastMinute = -1;

    for (;;) {
        if (currentPage == 1) { // 只在运行时间页面更新
            unsigned long seconds = (millis() / 1000);
            unsigned long minutes = seconds / 60;
            int currentMinute = minutes % 60;

            if (currentMinute != lastMinute) {
                lastMinute = currentMinute;

                // 获取显示互斥锁，更新时间显示
                if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(100))) {
                    refreshTimeDisplay();
                    xSemaphoreGive(xDisplayMutex);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(INTERVAL_CLOCK));
    }

    vTaskDelete(nullptr);
}

// ==================== 传感器读取任务 ====================
void sensor_task(void *pvParameters) {
    for (;;) {
        // 这里你可以填写实际的传感器读取代码
        // 示例数据，实际应用中替换成你的传感器读取
        SensorReading_t reading;
        reading.temperature = 25.5 + (rand() % 100) / 100.0;
        reading.humidity = 60.0 + (rand() % 200) / 10.0;
        reading.count = 100 + (millis() / 1000) / 60;
        reading.pressure = 1013.25 + (rand() % 100) / 10.0;
        reading.co2 = 400 + rand() % 50;
        reading.timestamp = millis();

        // 将读取的数据发送到队列，供显示任务和 BLE 任务使用
        // 如果队列满，覆盖旧数据（这里用阻塞等待 10ms）
        if (xQueueSend(xSensorDataQueue, &reading, pdMS_TO_TICKS(10))) {
            // 发送成功
        }

        vTaskDelay(pdMS_TO_TICKS(INTERVAL_SENSOR));
    }

    vTaskDelete(nullptr);
}

// ==================== 显示更新任务 ====================
void display_task(void *pvParameters) {
    // 初始化：显示默认页面
    if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY)) {
        display.clear();
        if (currentPage == 0) {
            page1();
        } else {
            page2();
        }
        xSemaphoreGive(xDisplayMutex);
    }

    for (;;) {
        ButtonEvent_t event;

        // 等待按键事件，阻塞等待（无限超时）
        if (xQueueReceive(xButtonEventQueue, &event, portMAX_DELAY)) {
            // 收到按键事件，处理它
            bool wasScreenOff = !display.isScreenOn();

            // 更新活动时间（用于自动息屏）
            if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(100))) {
                display.updateActivity();

                if (wasScreenOff) {
                    // 唤醒屏幕，重绘当前页面
                    display.wake();
                    display.clear();
                    if (currentPage == 0) {
                        page1();
                    } else {
                        page2();
                    }
                    Serial.println("Screen woken up");
                } else {
                    // 切换页面
                    currentPage = (currentPage + 1) % 2;
                    Serial.print("Switched to page: ");
                    Serial.println(currentPage);
                    display.clear();
                    if (currentPage == 0) {
                        page1();
                    } else {
                        page2();
                    }
                }

                xSemaphoreGive(xDisplayMutex);
            }
        }

        // 检查自动息屏（即使没有按键事件也要定期检查）
        // 这里每处理完一个按键事件就检查一次，比轮询更高效
        // 只有当距离上次检查已经过去了一定时间，才需要检查
        static unsigned long lastCheck = 0;
        unsigned long now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (display.isScreenOn() && (now - lastCheck > 1000)) {
            lastCheck = now;
            if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(100))) {
                display.checkSleep(10000);
                xSemaphoreGive(xDisplayMutex);
            }
        }
    }

    vTaskDelete(nullptr);
}

// ==================== BLE 通信任务 ====================
void ble_task(void *pvParameters) {
    // BLE 已经在 setup 中初始化，这里只需要定期发送数据
    SensorReading_t lastReading;
    memset(&lastReading, 0, sizeof(lastReading));

    for (;;) {
        // 检查是否有新的传感器数据
        if (BLEManager::isConnected()) {
            // 尝试从队列读取最新的传感器数据（非阻塞）
            // 持续读取直到队列为空，保留最新的一个
            while (xQueueReceive(xSensorDataQueue, &lastReading, 0)) {
                // 循环读取，最后一个就是最新的
            }

            // 发送数据到 BLE 客户端（二进制格式，带时间戳）
            // 这样从机可以直接解析结构体，保证数据同步
            if (lastReading.timestamp > 0) {
                // 更新时间戳为当前发送时间
                lastReading.timestamp = millis();
                BLEManager::sendSensorData((uint8_t*)&lastReading, sizeof(lastReading));
                Serial.printf("BLE: Sent sensor data, temp=%.1f hum=%.1f timestamp=%lu\n",
                           lastReading.temperature,
                           lastReading.humidity,
                           lastReading.timestamp);
            }
        } else {
            // 断开连接时，让 BLE 栈做维护工作
            BLEManager::update();
        }

        vTaskDelay(pdMS_TO_TICKS(INTERVAL_BLE));
    }

    vTaskDelete(nullptr);
}

// ==================== RTOS 初始化 ====================
void rtos_init(void) {
    // 创建互斥锁
    xDisplayMutex = xSemaphoreCreateMutex();
    xSensorDataMutex = xSemaphoreCreateMutex();

    // 创建队列
    // 按键事件队列：长度 5 足够，因为按键不会太快
    xButtonEventQueue = xQueueCreate(5, sizeof(ButtonEvent_t));
    // 传感器数据队列：长度 8 足够缓冲
    xSensorDataQueue = xQueueCreate(8, sizeof(SensorReading_t));

    // 创建各个任务
    // Arduino 核心已经在 Core 0 运行，我们把任务都创建到 Core 1
    // 如果你想让某个任务在 Core 0 运行，就把最后一个参数改成 0
    xTaskCreatePinnedToCore(button_task, "Button", TASK_STACK_BUTTON, nullptr,
                            TASK_PRIORITY_BUTTON, &xButtonTaskHandle, 1);

    xTaskCreatePinnedToCore(clock_task, "Clock", TASK_STACK_CLOCK, nullptr,
                            TASK_PRIORITY_CLOCK, &xClockTaskHandle, 1);

    xTaskCreatePinnedToCore(sensor_task, "Sensor", TASK_STACK_SENSOR, nullptr,
                            TASK_PRIORITY_SENSOR, &xSensorTaskHandle, 1);

    xTaskCreatePinnedToCore(display_task, "Display", TASK_STACK_DISPLAY, nullptr,
                            TASK_PRIORITY_DISPLAY, &xDisplayTaskHandle, 1);

    // BLE 任务默认启用
    xTaskCreatePinnedToCore(ble_task, "BLE", TASK_STACK_BLE, nullptr,
                            TASK_PRIORITY_BLE, &xBLETaskHandle, 1);

    Serial.println("All FreeRTOS tasks created");
}

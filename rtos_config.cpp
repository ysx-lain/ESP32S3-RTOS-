/**
 * @file rtos_config.cpp
 * @brief FreeRTOS 任务实现
 * @details 将各个功能模块划分为独立的 FreeRTOS 任务
 *
 *  任务架构:
 *  - 按键任务:扫描按键, 发送事件到队列
 *  - 时钟任务:每分钟更新时间
 *  - 传感器任务:定期读取传感器, 发送数据到队列
 *  - 显示任务:接收按键事件和传感器数据, 更新显示
 *  - BLE 任务:如果连接, 定期发送传感器数据
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
extern void page3();
extern void page1_update();
extern void refreshTimeDisplay();
extern unsigned long lastButtonPress;
extern const int PAGE_COUNT;

// ==================== 滚动相关 ====================
// 针对80px高度屏幕，内容放不下，支持按键滚动
int scrollOffset = 0;
const int scrollStep = 10;      // 每次滚动10像素
const int maxScrollOffset = 80; // 最大滚动不超过屏幕高度

// ==================== 常量定义 ====================
const unsigned long debounceTime = 200;  // 按键消抖时间(毫秒)

// ==================== 按键扫描任务 ====================
// 三个独立按键，分别功能:
//  - BUTTON_PIN_PAGE → 切换页面
//  - BUTTON_PIN_SCROLL_UP → 向上滚动
//  - BUTTON_PIN_SCROLL_DOWN → 向下滚动
void button_task(void *pvParameters) {
    // 初始化：读取初始状态
    int lastStatePage = digitalRead(BUTTON_PIN_PAGE);
    int lastStateUp = digitalRead(BUTTON_PIN_SCROLL_UP);
    int lastStateDown = digitalRead(BUTTON_PIN_SCROLL_DOWN);

    // 设置引脚模式：输入上拉
    pinMode(BUTTON_PIN_PAGE, INPUT_PULLUP);
    pinMode(BUTTON_PIN_SCROLL_UP, INPUT_PULLUP);
    pinMode(BUTTON_PIN_SCROLL_DOWN, INPUT_PULLUP);

    for (;;) { // 任务主循环
        // 1. 检测页面切换按键（下降沿）
        int buttonStatePage = digitalRead(BUTTON_PIN_PAGE);
        if (buttonStatePage == LOW && lastStatePage == HIGH) {
            unsigned long now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - lastButtonPress > debounceTime) {
                lastButtonPress = now;
                // 发送页面切换事件：isLongPress = false 表示切换页面
                ButtonEvent_t event;
                event.timestamp = now;
                event.isLongPress = false; // false = 切换页面
                xQueueSend(xButtonEventQueue, &event, 0);
            }
        }
        lastStatePage = buttonStatePage;

        // 2. 检测向上滚动按键（下降沿）
        int buttonStateUp = digitalRead(BUTTON_PIN_SCROLL_UP);
        if (buttonStateUp == LOW && lastStateUp == HIGH) {
            unsigned long now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - lastButtonPress > debounceTime) {
                lastButtonPress = now;
                // 向上滚动：直接修改偏移，发送重绘事件
                if (scrollOffset > 0) {
                    scrollOffset -= scrollStep;
                    if (scrollOffset < 0) scrollOffset = 0;
                    // 发送事件触发重绘
                    ButtonEvent_t event;
                    event.timestamp = now;
                    event.isLongPress = true; // true = 向上滚动
                    xQueueSend(xButtonEventQueue, &event, 0);
                }
            }
        }
        lastStateUp = buttonStateUp;

        // 3. 检测向下滚动按键（下降沿）
        int buttonStateDown = digitalRead(BUTTON_PIN_SCROLL_DOWN);
        if (buttonStateDown == LOW && lastStateDown == HIGH) {
            unsigned long now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - lastButtonPress > debounceTime) {
                lastButtonPress = now;
                // 向下滚动：直接修改偏移，发送重绘事件
                if (scrollOffset < maxScrollOffset) {
                    scrollOffset += scrollStep;
                    if (scrollOffset > maxScrollOffset) scrollOffset = maxScrollOffset;
                    // 发送事件触发重绘
                    ButtonEvent_t event;
                    event.timestamp = now;
                    event.isLongPress = false; // 复用isLongPress = false 表示向下滚动
                    xQueueSend(xButtonEventQueue, &event, 0);
                }
            }
        }
        lastStateDown = buttonStateDown;

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
    static float base_temp = 25.0f;      // 温度 ℃
    static float base_hum = 50.0f;       // 湿度 %RH
    static float base_press = 1013.0f;    // 气压 hPa
    static int base_co2 = 420;           // CO2 浓度 ppm
    static float base_ozone = 30.0f;     // 臭氧 O3 ppb
    static float base_acetaldehyde = 5.0f; // 乙醛 C2H4O ppb
    static float base_ethylene = 0.5f;    // 乙烯 C2H4 ppm
    static int counter = 0;

    // 启动时打印结构体大小
    Serial.print("[SENSOR] SensorReading_t size = ");
    Serial.print(sizeof(SensorReading_t));
    Serial.println(" bytes (expected 36 bytes)");

    for (;;) {
        // 模拟生成传感器数据，每次随机微小变化
        SensorReading_t reading;
        
        // 温度：20-35 ℃ 之间随机游走
        base_temp += ((rand() % 100) - 50) / 100.0f;
        if (base_temp < 20) base_temp = 20;
        if (base_temp > 35) base_temp = 35;
        reading.temperature = base_temp;
        
        // 湿度：30-80 %RH 之间随机游走
        base_hum += ((rand() % 100) - 50) / 10.0f;
        if (base_hum < 30) base_hum = 30;
        if (base_hum > 80) base_hum = 80;
        reading.humidity = base_hum;
        
        // 气压：980-1040 hPa 之间随机游走
        base_press += ((rand() % 100) - 50) / 10.0f;
        if (base_press < 980) base_press = 980;
        if (base_press > 1040) base_press = 1040;
        reading.pressure = base_press;
        
        // CO2：400-2000 ppm 之间随机游走
        base_co2 += (rand() % 21) - 10;
        if (base_co2 < 400) base_co2 = 400;
        if (base_co2 > 2000) base_co2 = 2000;
        reading.co2 = base_co2;
        
        // 臭氧：0-100 ppb 之间随机游走
        base_ozone += ((rand() % 100) - 50) / 10.0f;
        if (base_ozone < 0) base_ozone = 0;
        if (base_ozone > 100) base_ozone = 100;
        reading.ozone = base_ozone;
        
        // 乙醛：0-50 ppb 之间随机游走
        base_acetaldehyde += ((rand() % 100) - 50) / 100.0f;
        if (base_acetaldehyde < 0) base_acetaldehyde = 0;
        if (base_acetaldehyde > 50) base_acetaldehyde = 50;
        reading.acetaldehyde = base_acetaldehyde;
        
        // 乙烯：0-5 ppm 之间随机游走
        base_ethylene += ((rand() % 100) - 50) / 1000.0f;
        if (base_ethylene < 0) base_ethylene = 0;
        if (base_ethylene > 5) base_ethylene = 5;
        reading.ethylene = base_ethylene;
        
        // 计数器递增
        counter++;
        reading.count = counter;
        
        // 时间戳
        reading.timestamp = millis();

        // 更新全局最新传感器数据（显示任务直接用这个刷新）
        if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(100))) {
            latestSensorReading = reading;
            xSemaphoreGive(xSensorDataMutex);
        }

        // 将读取的数据发送到队列, 供 BLE 任务使用
        // 如果队列满, 覆盖旧数据(这里用阻塞等待 10ms)
        if (xQueueSend(xSensorDataQueue, &reading, pdMS_TO_TICKS(10))) {
            // 发送成功
        }

        vTaskDelay(pdMS_TO_TICKS(INTERVAL_SENSOR));
    }

    vTaskDelete(nullptr);
}

// ==================== 全局变量：保存最新传感器数据 ====================
// 显示任务需要直接读取最新数据来刷新
SensorReading_t latestSensorReading;

// ==================== 显示更新任务 ====================
void display_task(void *pvParameters) {
    // 初始化最新传感器数据为默认值
    latestSensorReading.temperature = 25.0f;
    latestSensorReading.humidity = 50.0f;
    latestSensorReading.pressure = 1013.0f;
    latestSensorReading.co2 = 420;
    latestSensorReading.ozone = 30.0f;
    latestSensorReading.acetaldehyde = 5.0f;
    latestSensorReading.ethylene = 0.5f;
    latestSensorReading.count = 0;
    latestSensorReading.timestamp = millis();

    // 初始化:显示默认页面
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
        // 超时 10ms 等待按键事件 → 这样传感器刷新更及时
        // 10Hz 刷新正好每 100ms 刷新 10 次，响应按键也及时
        ButtonEvent_t event;
        bool haveEvent = xQueueReceive(xButtonEventQueue, &event, pdMS_TO_TICKS(10));

        if (haveEvent) {
            // 收到按键事件，处理它
            bool wasScreenOff = !display.isScreenOn();

            // 更新活动时间(用于自动息屏)
            if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(100))) {
                display.updateActivity();

                if (wasScreenOff) {
                    // 唤醒屏幕，重绘当前页面
                    display.wake();
                    scrollOffset = 0; // 唤醒重置滚动
                    display.clear();
                    if (currentPage == 0) {
                        page1();  // page1() includes init
                    } else if (currentPage == 1) {
                        page2();
                    } else {
                        page3();
                    }
                    Serial.println("Screen woken up");
                } 
                else {
                    // 区分事件：
                    // - event.isLongPress = true  → 向上滚动
                    // - event.isLongPress = false → 
                    //   如果是原来的页面切换 → 现在改为：false = 向下滚动
                    if (!event.isLongPress) {
                        // 现在：false 两种情况：
                        // 1. 原来的页面切换 → 现在三个按键独立，所以：
                        //  - 页面切换按键按下 → 切换页面
                        //  - 向下滚动按键按下 → 滚动已经改了偏移，直接重绘
                        if (scrollOffset == 0 && event.isLongPress == false) {
                            // 是页面切换按键，切换页面
                            currentPage = (currentPage + 1) % PAGE_COUNT;
                            scrollOffset = 0; // 切换页面重置滚动偏移
                        }
                        // else 就是向下滚动，已经改了偏移，不用操作
                    }
                    // else 就是向上滚动，滚动已经改了偏移，不用操作

                    // 不管是什么事件，都重绘当前页面
                    display.clear();
                    if (currentPage == 0) {
                        page1();
                    } else if (currentPage == 1) {
                        page2();
                    } else {
                        page3();
                    }
                    Serial.printf("currentPage: %d, scrollOffset: %d\r\n", currentPage, scrollOffset);
                }

                xSemaphoreGive(xDisplayMutex);
            }
        }

        // 如果当前是传感器页面并且屏幕亮着，增量刷新数值
        // 只更新数值，不重绘标签，不全屏清屏，快很多不闪烁
        // 用短超时，不让按键事件等太久，保证响应及时
        // 刷新数据本身就是活动，更新activity时间，避免不必要息屏
        if (currentPage == 0 && display.isScreenOn()) {
            if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(10))) {
                page1_update();
                display.updateActivity(); // 数据刷新也算活动，更新最后活动时间
                xSemaphoreGive(xDisplayMutex);
            }
        }

        // 只要屏幕亮着，说明系统在运行，更新活动时间，只有真·空闲才息屏
        // 现在自动息屏已经禁用，这段保留不影响
        if (display.isScreenOn()) {
            if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(10))) {
                display.updateActivity(); // 每次循环都更新，保证任何页面只要亮着就不算空闲
                display.checkSleep(10000);
                xSemaphoreGive(xDisplayMutex);
            }
        }
    }

    vTaskDelete(nullptr);
}

// ==================== BLE 通信任务 ====================
void ble_task(void *pvParameters) {
    SensorReading_t reading;

    for (;;) {
        // 等待传感器数据队列新来的数据
        if (xQueueReceive(xSensorDataQueue, &reading, pdMS_TO_TICKS(INTERVAL_BLE))) {
            // 如果BLE已连接，发送数据
            // 格式化为JSON字符串发送
            char json[128];
            snprintf(json, sizeof(json), 
                "{\"temp\":%.1f,\"hum\":%.1f,\"press\":%.1f,\"co2\":%d,\"ozone\":%.1f,\"acetaldehyde\":%.2f,\"ethylene\":%.2f}",
                reading.temperature, reading.humidity, reading.pressure, 
                reading.co2, reading.ozone, reading.acetaldehyde, reading.ethylene);
            BLEManager::sendSensorData(json);
        }
    }

    vTaskDelete(nullptr);
}

// ==================== 初始化: 创建所有 FreeRTOS 对象 ====================
void rtos_init() {
    //  创建互斥锁：保护显示设备
    xDisplayMutex = xSemaphoreCreateMutex();
    // 创建互斥锁：保护传感器数据
    xSensorDataMutex = xSemaphoreCreateMutex();

    // 创建按键事件队列：长度为 5，足够存放多个按键事件
    xButtonEventQueue = xQueueCreate(5, sizeof(ButtonEvent_t));
    // 创建传感器数据队列：长度为 5，足够存放多个传感器读数
    xSensorDataQueue = xQueueCreate(5, sizeof(SensorReading_t));

    // 创建所有任务
    xTaskCreate(button_task,     "button_task",     TASK_STACK_BUTTON,     nullptr, TASK_PRIORITY_BUTTON,     &xButtonTaskHandle);
    xTaskCreate(clock_task,      "clock_task",      TASK_STACK_CLOCK,      nullptr, TASK_PRIORITY_CLOCK,      &xClockTaskHandle);
    xTaskCreate(sensor_task,     "sensor_task",     TASK_STACK_SENSOR,     nullptr, TASK_PRIORITY_SENSOR,     &xSensorTaskHandle);
    xTaskCreate(display_task,    "display_task",    TASK_STACK_DISPLAY,    nullptr, TASK_PRIORITY_DISPLAY,    &xDisplayTaskHandle);
    xTaskCreate(ble_task,       "ble_task",       TASK_STACK_BLE,       nullptr, TASK_PRIORITY_BLE,       &xBLETaskHandle);

    // 任务创建完就自动启动，不需要额外操作
    Serial.println("[RTOS] All tasks created successfully");
}

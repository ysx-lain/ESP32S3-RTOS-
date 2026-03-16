/**
 * @file rtos_config.h
 * @brief FreeRTOS 任务配置头文件
 * @details 定义任务优先级、栈大小、任务句柄
 *
 * 任务划分：
 *  - 按键扫描任务 (低优先级)
 *  - 屏幕刷新/显示任务 (中优先级)
 *  - 传感器读取任务 (中优先级)
 *  - BLE 通信任务 (中高优先级)
 *  - 时间更新任务 (低优先级)
 *
 * @author ysx
 * @date 2024-03-16
 */

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// ==================== 任务优先级定义 (0-24, 数字越大优先级越高) ====================
#define TASK_PRIORITY_BUTTON    1    // 按键扫描任务优先级
#define TASK_PRIORITY_CLOCK     1    // 时间更新任务优先级
#define TASK_PRIORITY_SENSOR    2    // 传感器读取任务优先级
#define TASK_PRIORITY_DISPLAY   2    // 屏幕显示任务优先级
#define TASK_PRIORITY_BLE       3    // BLE 通信任务优先级

// ==================== 任务栈大小定义 (单位：字，每个字 4 字节) ====================
#define TASK_STACK_BUTTON      2048  // 按键任务栈 (8KB)
#define TASK_STACK_CLOCK       2048  // 时间任务栈 (8KB)
#define TASK_STACK_SENSOR      4096  // 传感器任务栈 (16KB)
#define TASK_STACK_DISPLAY     8192  // 显示任务栈 (32KB) - Ucglib 需要较大栈
#define TASK_STACK_BLE         4096  // BLE 任务栈 (16KB)

// ==================== 任务轮询间隔 (毫秒) ====================
#define INTERVAL_BUTTON        10    // 按键扫描间隔 (10ms) - 响应及时
#define INTERVAL_CLOCK         1000  // 时间更新间隔 (1秒)
#define INTERVAL_SENSOR        1000  // 传感器读取间隔 (1秒)
#define INTERVAL_BLE            1000 // BLE 数据发送间隔 (1秒)

// ==================== 任务句柄 (用于任务管理) ====================
extern TaskHandle_t xButtonTaskHandle;
extern TaskHandle_t xClockTaskHandle;
extern TaskHandle_t xSensorTaskHandle;
extern TaskHandle_t xDisplayTaskHandle;
extern TaskHandle_t xBLETaskHandle;

// ==================== 互斥锁 (保护共享资源) ====================
// 显示设备是共享资源，多个任务访问需要互斥
extern SemaphoreHandle_t xDisplayMutex;
// 传感器数据结构体也可能被多个任务访问
extern SemaphoreHandle_t xSensorDataMutex;

// ==================== 消息队列 (任务间通信) ====================
// 按键事件队列：发送按键按下事件到显示任务
typedef struct {
    unsigned long timestamp;
    bool isLongPress;
} ButtonEvent_t;
extern QueueHandle_t xButtonEventQueue;

// 传感器数据队列：传感器任务把读取的数据发送给显示任务和 BLE 任务
#define MAX_SENSOR_COUNT 5
typedef struct {
    float temperature;      // 温度
    float humidity;          // 湿度
    int count;              // 计数
    float pressure;         // 气压
    int co2;                // CO2
    unsigned long timestamp;
} SensorReading_t;
extern QueueHandle_t xSensorDataQueue;

// ==================== 函数声明 ====================
void rtos_init(void);           // 初始化 RTOS：创建互斥锁、队列、任务
void rtos_start(void);          // 启动所有任务（实际上创建后就启动了）
void button_task(void *pvParameters);
void clock_task(void *pvParameters);
void sensor_task(void *pvParameters);
void display_task(void *pvParameters);
void ble_task(void *pvParameters);

// ==================== 硬件引脚定义 ====================
// 如果主程序没有定义，这里提供默认定义
#ifndef BUTTON_PIN
#define BUTTON_PIN 2
#endif
#ifndef BL_PIN
#define BL_PIN     12
#endif

// ==================== 常量定义 ====================
// 按键消抖时间（毫秒）
extern const unsigned long debounceTime;

#endif // RTOS_CONFIG_H

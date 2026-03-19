/**
 * @file rtos_config.h
 * @brief FreeRTOS 任务配置头文件
 * @details 定义任务优先级, 栈大小, 任务句柄
 *
 * 任务划分:
 *  - 按键中断任务 (高优先级，通过队列接收ISR事件)
 *  - 时钟任务:每分钟更新时间
 *  - 传感器读取任务:定期读取传感器, 发送数据到队列
 *  - 显示任务:接收按键事件和传感器数据, 更新显示
 *  - BLE 任务:如果连接, 定期发送传感器数据
 *
 * @author ysx
 * @date 2024-03-19
 */

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// ==================== 任务优先级定义 (0-24, 数字越大优先级越高) ====================
#define TASK_PRIORITY_BUTTON    3    // 按键任务优先级 (最高，保证及时响应)
#define TASK_PRIORITY_BLE       2    // BLE 通信任务优先级
#define TASK_PRIORITY_DISPLAY   2    // 屏幕显示任务优先级
#define TASK_PRIORITY_SENSOR    1    // 传感器读取任务优先级 (低，不抢按键响应)
#define TASK_PRIORITY_CLOCK     1    // 时间更新任务优先级 (低，定时更新)

// ==================== 任务栈大小定义 (单位:字, 每个字 4 字节) ====================
#define TASK_STACK_BUTTON      2048  // 按键任务栈 (8KB)
#define TASK_STACK_CLOCK       2048  // 时间任务栈 (8KB)
#define TASK_STACK_SENSOR      4096  // 传感器任务栈 (16KB)
#define TASK_STACK_DISPLAY     8192  // 显示任务栈 (32KB) - Ucglib 需要较大栈
#define TASK_STACK_BLE         4096  // BLE 任务栈 (16KB)

// ==================== 任务轮询间隔 (毫秒) ====================
#define INTERVAL_BUTTON        10    // 按键扫描间隔 (10ms) - 响应及时
#define INTERVAL_CLOCK         1000  // 时间更新间隔 (1秒)
#define INTERVAL_SENSOR        500   // 传感器读取间隔 (0.5秒 = 500ms) - 减少抢锁，不影响按键响应
#define INTERVAL_BLE            500   // BLE 数据发送间隔 (0.5秒 = 500ms)

// ==================== 任务句柄 (用于任务管理) ====================
extern TaskHandle_t xButtonTaskHandle;
extern TaskHandle_t xClockTaskHandle;
extern TaskHandle_t xSensorTaskHandle;
extern TaskHandle_t xDisplayTaskHandle;
extern TaskHandle_t xBLETaskHandle;

// ==================== 互斥锁 (保护共享资源) ====================
extern SemaphoreHandle_t xDisplayMutex;
extern SemaphoreHandle_t xSensorDataMutex;

// ==================== 外部全局变量声明 ====================
extern Display display;
extern int currentPage;
extern int scrollOffset;  // 滚动偏移，用于.ino文件中的页面绘制

// ==================== 外部函数声明 ====================
// page1() page2() page3() 定义在 sketch_feb26a.ino
void page1();
void page2();
void page3();
void page1_update();
void refreshTimeDisplay();
extern unsigned long lastButtonPress;
extern const int PAGE_COUNT;

// ==================== 按键事件队列声明 ====================
typedef struct {
    unsigned long timestamp;
    bool isLongPress; // true=向上滚动, false=向下滚动, 页面切换也用false
} ButtonEvent_t;
extern QueueHandle_t xButtonEventQueue;

// ==================== 传感器数据队列声明 ====================
typedef struct {
    float temperature;      // 温度 ℃
    float humidity;          // 湿度 %RH
    int count;              // 计数
    float pressure;         // 气压 hPa
    int co2;                // CO₂ 浓度 ppm
    float ozone;             // 臭氧 O₃ ppb
    float acetaldehyde;     // 乙醛 C₂H₄O ppb
    float ethylene;          // 乙烯 C₂H₄ ppm
    unsigned long timestamp; // 主机时间戳（毫秒）
} SensorReading_t;
extern QueueHandle_t xSensorDataQueue;

// ==================== 保存最新传感器数据 ====================
extern SensorReading_t latestSensorReading;

// ==================== 函数声明 ====================
void rtos_init(void);           // 初始化 RTOS:创建互斥锁, 队列, 任务
void rtos_start(void);          // 启动所有任务(实际上创建后就启动了)

// ==================== 中断服务例程(ISR)声明 ====================
// 按键中断触发：三个独立ISR
extern void IRAM_ATTR button_isr_page();    // GPIO4: 页面切换
extern void IRAM_ATTR button_isr_up();     // GPIO5: 向上滚动
extern void IRAM_ATTR button_isr_down();   // GPIO6: 向下滚动

// ==================== 任务函数声明 ====================
void button_task(void *pvParameters);
void clock_task(void *pvParameters);
void sensor_task(void *pvParameters);
void display_task(void *pvParameters);
void ble_task(void *pvParameters);

// ==================== 硬件引脚定义 ====================
// 三个按键:
// - BUTTON_PIN_PAGE → 切换页面
// - BUTTON_PIN_SCROLL_UP → 向上滚动
// - BUTTON_PIN_SCROLL_DOWN → 向下滚动
#ifndef BUTTON_PIN_PAGE
#define BUTTON_PIN_PAGE 4
#endif
#ifndef BUTTON_PIN_SCROLL_UP
#define BUTTON_PIN_SCROLL_UP 5
#endif
#ifndef BUTTON_PIN_SCROLL_DOWN
#define BUTTON_PIN_SCROLL_DOWN 6
#endif
#ifndef BL_PIN
#define BL_PIN     12
#endif

// ==================== 常量定义 ====================
// 按键消抖时间(毫秒)
extern const unsigned long debounceTime;

// 总页面数量(定义在 sketch_feb26a.ino)
extern const int PAGE_COUNT;

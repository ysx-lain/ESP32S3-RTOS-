/**
 * @file Config.h
 * @brief 项目全局配置头文件
 *
 * 统一配置：任务栈大小、任务优先级、硬件引脚、编译选项
 * 便于修改和维护
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==================== 硬件引脚定义 ====================
#define BUTTON_PIN      2       // 按键引脚
#define BL_PIN          12      // 背光控制引脚
#define SCREEN_SCLK     13      // 屏幕时钟
#define SCREEN_MOSI     11      // 屏幕数据
#define SCREEN_CD       9       // 屏幕命令/数据
#define SCREEN_CS       10      // 屏幕片选
#define SCREEN_RESET    8       // 屏幕复位

// ==================== FreeRTOS 任务配置 ====================
// 栈大小（单位：字，每个字 4 字节）
#define TASK_STACK_BUTTON    2048    // 按键任务 → 8KB
#define TASK_STACK_CLOCK     2048    // 时钟任务 → 8KB
#define TASK_STACK_SENSOR    4096    // 传感器任务 → 16KB
#define TASK_STACK_DISPLAY   4096    // 显示任务 → 16KB
#define TASK_STACK_BLE       4096    // BLE 任务 → 16KB

// 任务优先级（数字越大优先级越高）
#define TASK_PRIORITY_BUTTON   2    // 按键 → 优先级 2
#define TASK_PRIORITY_CLOCK    2    // 时钟 → 优先级 2
#define TASK_PRIORITY_SENSOR   3    // 传感器采集 → 优先级 3
#define TASK_PRIORITY_DISPLAY  3    // 显示更新 → 优先级 3
#define TASK_PRIORITY_BLE      4    // BLE 通信 → 优先级 4

// ==================== 定时/间隔配置 ====================
#define INTERVAL_BUTTON     20      // 按键扫描间隔（ms）
#define INTERVAL_CLOCK      60000   // 时钟更新间隔（ms）
#define INTERVAL_SENSOR     100     // 传感器采集间隔（ms）→ 10Hz
#define INTERVAL_BLE        100     // BLE 发送间隔（ms）→ 10Hz
#define SCREEN_TIMEOUT      10000   // 自动息屏超时（ms）→ 10 秒

// ==================== 队列配置 ====================
#define QUEUE_LENGTH_BUTTON     5    // 按键事件队列长度
#define QUEUE_LENGTH_SENSOR     8    // 传感器数据队列长度

// ==================== 传感器范围配置 ====================
#define SENSOR_TEMP_MIN     20.0f   // 温度最小值
#define SENSOR_TEMP_MAX     35.0f   // 温度最大值
#define SENSOR_HUM_MIN      30.0f   // 湿度最小值
#define SENSOR_HUM_MAX      80.0f   // 湿度最大值
#define SENSOR_PRESS_MIN    980.0f  // 气压最小值
#define SENSOR_PRESS_MAX   1040.0f  // 气压最大值
#define SENSOR_CO2_MIN     400      // CO2 最小值 ppm
#define SENSOR_CO2_MAX    2000      // CO2 最大值 ppm
#define SENSOR_OZONE_MIN    0.0f    // 臭氧最小值 ppb
#define SENSOR_OZONE_MAX  100.0f    // 臭氧最大值 ppb
#define SENSOR_ACETAL_MIN   0.0f    // 乙醛最小值 ppb
#define SENSOR_ACETAL_MAX  50.0f    // 乙醛最大值 ppb
#define SENSOR_ETHYLENE_MIN 0.0f    // 乙烯最小值 ppm
#define SENSOR_ETHYLENE_MAX 5.0f    // 乙烯最大值 ppm

// ==================== BLE 配置 ====================
#define BLE_DEVICE_NAME     "ESP32-S3_Sensor"
#define BLE_SERVICE_UUID    "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CHAR_UUID_TX    "1b9a473a-4493-4536-8b2b-9d4133488256"
#define BLE_CHAR_UUID_RX    "2c9b473a-5594-4736-9b2b-8d5234489357"

// ==================== 日志宏定义 ====================
#ifdef ENABLE_DEBUG_LOG
#define LOG_INFO(fmt, ...)   Serial.printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   Serial.printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)  Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)   // 禁用时为空
#define LOG_WARN(fmt, ...)   // 禁用时为空
#define LOG_ERROR(fmt, ...)  // 禁用时为空
#endif

// 默认开启调试日志
#ifndef ENABLE_DEBUG_LOG
#define ENABLE_DEBUG_LOG 1
#endif

#endif // CONFIG_H

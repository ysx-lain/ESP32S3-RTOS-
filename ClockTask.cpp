/**
 * @file ClockTask.cpp
 * @brief 时钟/系统运行时间任务实现
 *
 * 功能：
 * - 每分钟更新一次运行时间
 * - 提供强制刷新函数用于页面切换
 */

#include "ClockTask.h"
#include "Config.h"
#include "Display.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 外部声明全局显示对象
extern Display display;

/**
 * @brief 强制刷新运行时间显示（用于页面切换时立即更新）
 */
void refreshTimeDisplay(void) {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Uptime: %lud %02lu:%02lu", days, hours % 24, minutes % 60);

    const int fontHeight = 13;       // FONT_MEDIUM_SIZE 高度
    const int baselineY = 50;
    const int clearX = 10;
    const int clearY = baselineY - fontHeight;
    const int clearW = 150;
    const int clearH = fontHeight + 2;

    display.fillRect(clearX, clearY, clearW, clearH, 0, 0, 0);
    display.drawText(10, baselineY, buffer, FONT_MEDIUM_SIZE, 255, 255, 255);
    LOG_DEBUG("Time display refreshed: %s", buffer);
}

/**
 * @brief 时钟任务入口函数
 */
void clock_task(void *pvParameters) {
    LOG_INFO("Clock task started");

    for (;;) {
        // 每分钟更新一次，不需要高频率
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_CLOCK));
    }

    vTaskDelete(nullptr);
}

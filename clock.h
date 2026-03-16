#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <Ticker.h>

/**
 * SystemClock - 系统时钟管理类
 * 使用 Ticker 产生每秒中断，维护一个递增的秒计数器。
 * 主循环可通过 getTicks() 获取当前秒数，并与上次值比较来执行周期性任务。
 */
class SystemClock {
private:
    static volatile uint32_t _ticks;       // 自启动以来的秒数（由中断更新）
    static Ticker _ticker;                  // 定时器对象

    static void IRAM_ATTR _tickHandler();   // 中断处理函数（递增计数器）

public:
    /**
     * 初始化系统时钟，启动 Ticker 每秒中断
     * 应在 setup() 中调用
     */
    static void begin();

    /**
     * 获取当前秒数（线程安全，读取时关中断）
     * @return 自 begin() 调用起的秒数
     */
    static uint32_t getTicks();
};

#endif
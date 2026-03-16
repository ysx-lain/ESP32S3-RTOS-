/**
 * @file clock.cpp
 * @brief 系统时钟模块实现文件（基于 Ticker 硬件定时器）
 * 
 * 本文件实现了 SystemClock 类的功能，利用 ESP32 的 Ticker 库
 * 产生每秒中断，维护一个递增的秒计数器，为系统提供精确的秒级心跳。
 * 该模块是传感器数据采集、定时上报和屏幕息屏等周期性任务的时间基准。
 * 
 * @note 使用 Ticker 硬件定时器，中断服务函数应保持简短，仅递增计数器。
 * 
 * @author 根据项目需求编写
 * @date 2024-03-02
 * @version 1.0
 */

#include "clock.h"

// 静态成员变量定义（分配存储空间）
volatile uint32_t SystemClock::_ticks = 0;   ///< 自启动以来的秒数，由中断更新，volatile 确保编译器不优化
Ticker SystemClock::_ticker;                  ///< Ticker 对象，用于绑定定时器中断

/**
 * @brief 定时器中断处理函数（每秒触发一次）
 * 
 * 该函数在中断上下文中执行，必须简短快速。
 * 采用 `_ticks = _ticks + 1` 而非 `_ticks++` 以避免对 volatile 变量的弃用警告。
 * 
 * @note 使用 IRAM_ATTR 将函数放入 IRAM 以提高中断响应速度。
 */
void IRAM_ATTR SystemClock::_tickHandler() {
    // 避免使用 _ticks++，因为对 volatile 的自增操作已被弃用
    _ticks = _ticks + 1;
}

/**
 * @brief 初始化系统时钟，启动 Ticker 定时器
 * 
 * 调用 Ticker 的 attach 方法，设置定时器每秒触发一次中断，
 * 中断处理函数为 _tickHandler。
 * 
 * @note 应在 setup() 函数中调用，且仅调用一次。
 */
void SystemClock::begin() {
    _ticker.attach(1.0, _tickHandler);
}

/**
 * @brief 获取当前系统运行的秒数（线程安全）
 * 
 * 通过关中断保护，确保读取 32 位变量 _ticks 时不会被中断打断，
 * 从而获得一致的值。读取后恢复中断。
 * 
 * @return uint32_t 自 begin() 调用以来经过的秒数（模 2^32，约 136 年溢出一次）
 */
uint32_t SystemClock::getTicks() {
    uint32_t ticks;
    noInterrupts();          // 关中断，保证读取原子性
    ticks = _ticks;
    interrupts();            // 恢复中断
    return ticks;
}
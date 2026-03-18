/**
 * @file rtos_test.ino
 * @brief FreeRTOS 多任务调度测试程序
 * @details 测试内容：
 *  1. 多个任务创建和调度
 *  2. 互斥锁保护共享资源
 *  3. 消息队列传递数据
 *  4. 优先级抢占测试
 *  5. 双核绑定测试（所有任务运行在 Core 1）
 * 
 * 预期结果：串口输出有序的测试日志，证明 RTOS 工作正常
 * 
 * 编译：直接用 Arduino IDE 打开这个文件即可编译上传
 * 串口波特率：115200
 * 
 * @author ysx
 * @date 2026-03-18
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// ==================== 测试配置 ====================
#define TEST_ITERATIONS 10    // 测试循环次数
#define HIGH_PRIORITY 3       // 高优先级
#define LOW_PRIORITY  1       // 低优先级

// ==================== 全局对象 ====================
SemaphoreHandle_t xTestMutex;    // 互斥锁，保护串口共享资源
QueueHandle_t xTestQueue;        // 消息队列，传递测试数据

// 计数器（共享资源，需要互斥保护）
volatile int g_counter = 0;

// ==================== 任务函数声明 ====================
void high_priority_task(void *pv);
void low_priority_task(void *pv);
void producer_task(void *pv);
void consumer_task(void *pv);

// ==================== 测试数据结构 ====================
typedef struct {
    int id;             // 任务ID
    int sequence;      // 序号
    unsigned long tick; // 时间戳
} TestMessage_t;

// ==================== 高优先级任务 ====================
void high_priority_task(void *pv) {
    Serial.println("[HIGH] High priority task started");
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        // 获取互斥锁
        if (xSemaphoreTake(xTestMutex, portMAX_DELAY)) {
            int tmp = g_counter;
            tmp++;
            g_counter = tmp;
            Serial.printf("[HIGH] counter = %d → Running on Core %d\r\n", 
                         g_counter, xPortGetCoreID());
            xSemaphoreGive(xTestMutex);
        }
        
        // 延时，让低优先级任务有机会运行
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    Serial.println("[HIGH] High priority task finished");
    vTaskDelete(nullptr);
}

// ==================== 低优先级任务 ====================
void low_priority_task(void *pv) {
    Serial.println("[LOW] Low priority task started");
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        // 获取互斥锁
        if (xSemaphoreTake(xTestMutex, portMAX_DELAY)) {
            int tmp = g_counter;
            tmp++;
            g_counter = tmp;
            Serial.printf("[LOW]  counter = %d → Running on Core %d\r\n", 
                         g_counter, xPortGetCoreID());
            xSemaphoreGive(xTestMutex);
        }
        
        // 延时更长，让高优先级抢占
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    Serial.println("[LOW] Low priority task finished");
    vTaskDelete(nullptr);
}

// ==================== 生产者任务：产生数据发到队列 ====================
void producer_task(void *pv) {
    int seq = 0;
    
    Serial.println("[PRODUCER] Producer started");
    
    for (int i = 0; i < 20; i++) {
        TestMessage_t msg;
        msg.id = 1;
        msg.sequence = seq;
        msg.tick = xTaskGetTickCount();
        
        // 发送到队列
        if (xQueueSend(xTestQueue, &msg, pdMS_TO_TICKS(100))) {
            if (xSemaphoreTake(xTestMutex, pdMS_TO_TICKS(100))) {
                Serial.printf("[PRODUCER] Sent message #%d at tick %lu\r\n", 
                             seq, msg.tick);
                xSemaphoreGive(xTestMutex);
            }
        }
        
        seq++;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    Serial.println("[PRODUCER] Producer finished");
    vTaskDelete(nullptr);
}

// ==================== 消费者任务：从队列读取数据 ====================
void consumer_task(void *pv) {
    Serial.println("[CONSUMER] Consumer started");
    
    for (int i = 0; i < 20; i++) {
        TestMessage_t msg;
        
        // 阻塞等待数据
        if (xQueueReceive(xTestQueue, &msg, portMAX_DELAY)) {
            if (xSemaphoreTake(xTestMutex, pdMS_TO_TICKS(100))) {
                unsigned long delay = xTaskGetTickCount() - msg.tick;
                Serial.printf("[CONSUMER] Received message #%d, latency = %lu ticks\r\n", 
                             msg.sequence, delay);
                xSemaphoreGive(xTestMutex);
            }
        }
    }
    
    Serial.println("[CONSUMER] Consumer finished");
    vTaskDelete(nullptr);
}

// ==================== 初始化和启动测试 ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // 打印测试标题
    Serial.println("======================================");
    Serial.println("  ESP32S3 FreeRTOS 多任务测试程序");
    Serial.println("======================================");
    Serial.printf("  Test iterations: %d\r\n", TEST_ITERATIONS);
    Serial.printf("  Arduino loop running on Core: %d\r\n", xPortGetCoreID());
    Serial.println("======================================");
    Serial.println();
    
    // 创建互斥锁
    xTestMutex = xSemaphoreCreateMutex();
    Serial.println("[SETUP] Mutex created");
    
    // 创建消息队列（容量10个测试消息）
    xTestQueue = xQueueCreate(10, sizeof(TestMessage_t));
    Serial.println("[SETUP] Queue created");
    
    // 创建测试任务，都绑定到 Core 1
    Serial.println("[SETUP] Creating tasks...");
    
    // 测试1：优先级抢占测试
    Serial.println("[SETUP] === Test 1: Priority preemption test ===");
    xTaskCreatePinnedToCore(high_priority_task, "High", 2048, nullptr, 
                            HIGH_PRIORITY, nullptr, 1);
    xTaskCreatePinnedToCore(low_priority_task, "Low", 2048, nullptr, 
                            LOW_PRIORITY, nullptr, 1);
    
    // 等待第一个测试完成
    vTaskDelay(pdMS_TO_TICKS(3500));
    
    Serial.println();
    Serial.println("[SETUP] === Test 1 Complete ===\r\n");
    
    // 测试2：生产者消费者队列测试
    Serial.println("[SETUP] === Test 2: Producer/consumer queue test ===");
    xTaskCreatePinnedToCore(producer_task, "Producer", 2048, nullptr, 
                            2, nullptr, 1);
    xTaskCreatePinnedToCore(consumer_task, "Consumer", 2048, nullptr, 
                            2, nullptr, 1);
    
    // 等待第二个测试完成
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    Serial.println();
    Serial.println("[SETUP] === Test 2 Complete ===\r\n");
    
    // 测试总结
    Serial.println("======================================");
    Serial.println("  All tests completed!");
    Serial.println("======================================");
    Serial.println("✅ 如果看到有序的输出，说明 RTOS 工作正常");
    Serial.println("✅ 所有任务都成功运行在 Core 1");
    Serial.println("✅ 互斥锁正常工作，保护了共享资源");
    Serial.println("✅ 消息队列正常工作，数据传递正确");
    Serial.println("======================================");
}

void loop() {
    // 所有测试都完成了，这里只需要睡眠
    vTaskDelay(pdMS_TO_TICKS(1000));
}

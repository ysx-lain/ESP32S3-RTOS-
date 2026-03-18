 # 开发说明：如何添加新功能（软件/硬件）

本项目基于 **FreeRTOS 多任务架构**，添加新功能很方便。本文档说明步骤。

---

## 目录
- [总体架构](#总体架构)
- [如何添加新的传感器](#如何添加新的传感器)
- [如何添加新任务](#如何添加新任务)
- [如何修改现有任务参数](#如何修改现有任务参数)
- [如何添加新硬件驱动](#如何添加新硬件驱动)
- [任务间通信说明](#任务间通信说明)

---

## 总体架构

```
┌─────────────┐     ┌──────────────┐     ┌──────────────┐
│ button_task│────▶│ 按键事件队列 │────▶│ display_task│
│  优先级 1  │     │(ButtonEvent) │     │  优先级 2   │
└─────────────┘     └──────────────┘     └──────────────┘
                          ↑                    ↑
                          │                    │
┌─────────────┐     ┌──────────────┐          │
│ sensor_task│────▶│ 传感器数据队列│          │
│  优先级 2  │     │(SensorReading)│          │
└─────────────┘     └──────────────┘          │
                          ↓                    │
┌─────────────┐                            │
│   ble_task  │◀───────────────────────────┘
│  优先级 3  │
└─────────────┘

┌─────────────┐
│  clock_task│ 每分钟刷新时间显示
│  优先级 1  │
└─────────────┘
```

- **每个功能一个独立任务**
- **队列**传递消息/数据，**互斥锁**保护共享资源
- 所有任务都创建在 **Core 1**，Arduino 核心在 Core 0 运行

---

## 如何添加新的传感器

### 步骤 1：修改 `sensor_task` 在 `rtos_config.cpp`

找到 `sensor_task()` 函数，在里面添加你的传感器读取代码：

```cpp
// 在 sensor_task 循环里
SensorReading_t reading;
// 你的读取代码
reading.temperature = your_sensor.readTemperature();
reading.humidity = your_sensor.readHumidity();
// ... 其他字段
reading.timestamp = millis();

// 发送到队列，给显示任务和 BLE 任务用
xQueueSend(xSensorDataQueue, &reading, pdMS_TO_TICKS(10));
```

### 步骤 2：修改显示页面 `page1()` 在 `sketch_feb26a.ino`

取消这里的注释，改成动态显示队列中的最新数据：

```cpp
void page1() {
    SensorReading_t reading;
    // 非阻塞读取，拿到最新数据
    while (xQueueReceive(xSensorDataQueue, &reading, 0)) {
        // 保留最新一个
    }
    display.Sensor(3, "Temp", reading.temperature, "C");
    display.Sensor(4, "Hum",  reading.humidity,   "%");
    display.Sensor(5, "Count", reading.count, NULL);
    display.Sensor(6, "Press", reading.pressure, "hPa");
    display.Sensor(7, "CO2",   reading.co2,  "ppm");
    // 如果有第 8 个传感器，再加一行
}
```

### 完成！

传感器任务会每秒读取一次，发送到队列，显示页面切换时显示最新数据，BLE 任务也会自动发送最新数据。

---

## 如何添加新任务

如果要加一个全新功能（比如电池检测、背光自动调节、计步等），按这个步骤：

### 步骤 1：在 `rtos_config.h` 添加声明

```cpp
// 添加任务句柄（放到其他任务句柄后面）
extern TaskHandle_t xYourTaskHandle;

// 添加任务参数（如果需要）
#define TASK_STACK_YOUR    2048  // 栈大小（字）
#define TASK_PRIORITY_YOUR 1     // 优先级（0-24，越大优先级越高）
#define INTERVAL_YOUR      1000  // 运行间隔（毫秒）

// 声明任务函数
void your_task(void *pvParameters);
```

### 步骤 2：在 `rtos_config.cpp` 实现任务函数

```cpp
void your_task(void *pvParameters) {
    // 这里做初始化（如果需要）
    // your_init();

    for (;;) { // 任务主循环必须是死循环
        // 你的任务代码...

        // 延时，让出CPU
        vTaskDelay(pdMS_TO_TICKS(INTERVAL_YOUR));
    }

    // 如果任务能退出，删除自己
    vTaskDelete(nullptr);
}
```

### 步骤 3：在 `rtos_init()` 创建任务

找到 `rtos_init()` 函数，在最后添加：

```cpp
xTaskCreatePinnedToCore(your_task, "YourTask", TASK_STACK_YOUR, nullptr,
                        TASK_PRIORITY_YOUR, &xYourTaskHandle, 1);
```

### 完成！

上电后任务会自动创建并运行。

---

## 如何修改现有任务参数

所有参数都在 **`rtos_config.h`** 集中定义，直接改就行：

| 参数 | 位置 | 说明 |
|------|------|------|
| 任务优先级 | `TASK_PRIORITY_*` | 数字越大优先级越高（0最低，24最高） |
| 任务栈大小 | `TASK_STACK_*` | 单位是 **字**，1字 = 4字节。简单任务 2048 (8KB)，复杂任务 4096 (16KB) |
| 运行间隔 | `INTERVAL_*` | 单位毫秒，每隔多久运行一次 |

例子：觉得按键响应慢 → 把 `TASK_PRIORITY_BUTTON` 从 1 改成 2。

---

## 如何添加新硬件驱动

1. 如果是独立驱动，新建 `your_driver.h` 和 `your_driver.cpp` 文件
2. 在需要使用的地方 `#include "your_driver.h"`
3. 如果需要一个独立任务读取/处理，按上面【添加新任务】步骤添加

例子：添加 **BQ27441 电池电量计**：
- 创建 `bq27441.h` + `bq27441.cpp`
- 添加 `battery_task` 到 `rtos_config`，每分钟读一次电量
- 把电量放到 `SensorReading_t` 里发显示

---

## 任务间通信说明

### 什么时候用队列？

- **事件传递**：按键按下 → 发送事件给显示任务 → `xButtonEventQueue`
- **数据传递**：传感器读取 → 发送给显示 + BLE → `xSensorDataQueue`

### 什么时候用互斥锁？

多个任务访问**同一个共享资源**必须加锁：

```cpp
// 访问共享资源（比如屏幕）之前拿锁
if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(100))) {
    // 操作共享资源...
    display.fillRect(...);
    // 操作完一定要释放锁！
    xSemaphoreGive(xDisplayMutex);
}
```

现在已经有的互斥锁：
- `xDisplayMutex` → 保护 `display` 对象，多个任务画屏幕必须用这个

### 常见问题

- **队列满了怎么办**：发送用 `xQueueSend(..., 10)` 超时 10ms，发不进去就丢弃，不阻塞
- **怎么拿到最新数据**：如果只要最新的，循环读直到队列为空：
  ```cpp
  SensorReading_t reading;
  while (xQueueReceive(xSensorDataQueue, &reading, 0)) {
      // 最后一个就是最新的
  }
  ```

---

## 调试建议

1. 如果死机/重启 → 很可能是栈溢出 → 增大对应任务的 `TASK_STACK_*`
2. 如果任务没响应 → 检查优先级是不是太低被高优先级任务抢占了
3. 如果屏幕花了 → 检查多个任务访问屏幕有没有拿互斥锁
4. 可以用 `uxTaskGetStackHighWaterMark(NULL)` 检查任务剩余栈：
   ```cpp
   UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
   Serial.printf("Stack remaining: %d words\n", remaining);
   ```

---

## 总结

| 要改什么 | 文件 |
|---------|------|
| 任务优先级/栈大小/间隔 | `rtos_config.h` |
| 任务逻辑 | `rtos_config.cpp` |
| 页面显示/全局变量/硬件引脚 | `sketch_feb26a.ino` |
| 新增驱动 | 新建 `driver.h/cpp` |
| BLE 配置 | `ble.h/cpp` |
| 显示驱动 | `Display.h/cpp` |

架构搭好了，放心加功能吧！

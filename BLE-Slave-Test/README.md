# BLE 从机测试程序

## 用途

用于测试主机（ESP32S3-RTOS）的传感器数据读取速度和 BLE 传输速度。
主机定时读取传感器数据（这里是模拟生成），通过 BLE 发送给从机，从机计算延迟和统计。

## 架构

```
[主机（手表）] --BLE--> [从机（测试）] --UART--> 串口输出统计
     ↓
  传感器任务每秒生成一次数据，放入队列
  BLE 任务每 5 秒从队列取出最新数据发送
  数据格式：二进制结构体，带时间戳
```

## 数据格式

```cpp
typedef struct {
    float temperature;      // 温度
    float humidity;          // 湿度
    int count;              // 计数
    float pressure;         // 气压
    int co2;                // CO2
    unsigned long timestamp; // 主机发送时间戳（毫秒）
} SensorReading;
```

总大小：`4 + 4 + 4 + 4 + 4 + 4 = 24 字节`

## 编译和烧录

1. 用 Arduino IDE 打开 `BLE-Slave-Test.ino`
2. 选择板型：ESP32S3
3. 编译烧录到你的另一块 ESP32S3 开发板

## 测试步骤

1. 在主机（手表）侧：
   - 在 `sketch_feb26a.ino` 取消 `#include "ble.h"` 注释
   - 在 `rtos_config.cpp` 取消 BLE 任务创建的注释
   - 编译烧录到手表，开机等待连接

2. 在从机（测试）侧：
   - 修改 `TARGET_DEVICE_NAME` 为你的主机 BLE 名称（默认 `"ESP32-S3_Sensor"`）
   - 编译烧录，打开串口监视器（115200 baud）
   - 从机会自动扫描并连接主机
   - 连接成功后开始接收数据，串口输出：
     ```
     === Received sensor data ===
     Temperature: 25.5 C
     Humidity:    60.0 %
     Pressure:    1013.25 hPa
     CO2:         400 ppm
     Count:       12
     Timestamp:   12345 ms (latency: 12 ms)

     --- Statistics ---
     Total packets: 10
     Errors:        0
     Latency min:   8 ms
     Latency max:   25 ms
     Latency avg:   12 ms
     ------------------
     ```

## 测试指标

- **数据同步**：从机接收的数据是否顺序正确，有没有丢包
- **传输延迟**：latency 是从主机时间戳到从机接收的时间差，反映 BLE 传输速度
- **错误率**：totalErrors 统计长度错误的包

## 注意事项

- 主机 BLE 服务 UUID 是 `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- 特征 TX UUID 是 `1b9a473a-4493-4536-8b2b-9d4133488256`
- 从机代码使用标准 Arduino BLE 库（不是 NimBLE），如果需要 NimBLE 可以修改

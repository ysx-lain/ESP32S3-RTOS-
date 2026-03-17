# ESP32S3 传感器数据 Windows 串口客户端

## 功能

1. **串口接收模式** - 连接 ESP32S3 主机串口，实时接收传感器数据并显示
2. **模拟生成模式** - 在 PC 上模拟生成传感器数据，0.1 秒生成一次，用于测试

## 编译生成 EXE

### 使用 dotnet 命令行：

```bash
# 生成 64 位 Windows 独立 exe
dotnet publish -c Release -r win-x64 --self-contained true

# 生成的 exe 在 bin/Release/net6.0/win-x64/publish/Esp32SensorClient.exe
```

### 使用 Visual Studio：

打开 `Esp32SensorClient.csproj` → 右键项目 → 发布 → 选择自包含 win-x64 → 发布

## 使用方法

### 方式 1：串口接收

直接双击运行 `Esp32SensorClient.exe`，按提示选择串口和波特率（默认 115200），然后开始接收。

### 方式 2：模拟生成

```cmd
Esp32SensorClient.exe simulate
```

会在控制台输出模拟传感器数据，0.1 秒生成一次，按 Ctrl+C 停止并显示统计。

## 数据格式

和 ESP32 端 `SensorReading` 结构体完全一致：

| 字段 | 类型 | 说明 |
|------|------|------|
| temperature | float | 温度 ℃ |
| humidity | float | 湿度 %RH |
| count | int | 计数 |
| pressure | float | 气压 hPa |
| co2 | int | CO2浓度 ppm |
| timestamp | uint | 主机时间戳 ms |

## 输出

- 实时打印每一包数据
- 定期输出统计信息（总包数、错误数、包每秒速率）


# ESP32S3 FreeRTOS 多任务 BLE 传感器项目

基于 Arduino 框架 + FreeRTOS 多任务架构的 ESP32S3 传感器采集项目，支持网页串口客户端，无需编译直接使用。

## 📋 项目特点

- ✅ **规范 FreeRTOS 架构**：每个功能一个独立任务，统一栈大小/优先级配置
- ✅ **多页面显示**：传感器数据 / 运行时间 / 系统状态 三页循环切换
- ✅ **10Hz 采集 + 发送**：传感器每 100ms 更新一次
- ✅ **二进制传输**：直接发送结构体，网页客户端直接解析
- ✅ **网页串口客户端**：基于 Web Serial API，无需编译，Chrome/Edge 直接打开使用
- ✅ **模拟数据**：无硬件时可在网页模拟四种气体变化
- ✅ **自动息屏**：10秒无操作自动关闭背光，按键唤醒
- ✅ **完整日志**：LOG 宏封装，方便调试

## 🔧 硬件接线

| 功能       | ESP32S3 GPIO | 说明                |
|------------|-------------|---------------------|
| 按键       | GPIO2       | 接 GND，内部上拉   |
| 背光       | GPIO12      | 高电平点亮          |
| 屏幕 SCLK  | GPIO13      | SPI 时钟            |
| 屏幕 MOSI  | GPIO11      | SPI 数据            |
| 屏幕 CD    | GPIO9       | 命令/数据           |
| 屏幕 CS    | GPIO10      | 片选                |
| 屏幕 RESET | GPIO8       | 复位                |

## 📦 依赖库安装

Arduino IDE 需要安装：
1. **ESP32 Arduino Core**：版本 2.0.11+
2. **NimBLE-Arduino**：通过 Arduino 库管理器安装
3. **Ucglib**：通过 Arduino 库管理器安装（虽然提示架构不兼容，但可以正常使用）

## 📁 项目结构

```
ESP32S3-RTOS/
├── ESP32S3-RTOS.ino  ← 主入口，只放初始化
├── Config.h            ← 全局配置（引脚、栈大小、优先级等
├── TaskInit.h/cpp      ← 所有任务初始化
├── ButtonTask.h/cpp    ← 按键扫描任务
├── ClockTask.h/cpp     ← 时钟/运行时间任务
├── SensorTask.h/cpp    ← 传感器采集任务（模拟数据）
├── DisplayTask.h/cpp  ← 显示更新任务
├── BLETask.h/cpp      ← BLE 通信任务
├── ble.h/cpp           ← BLE 模块封装
├── Display.h/cpp       ← 屏幕驱动封装
├── web-serial-client/  ← 网页串口客户端（无需编译
│   └── index.html
└── README.md
```

## 🚀 使用方法

### 1. 编译上传固件

1. 克隆项目到 Arduino IDE 项目文件夹
2. 打开 `ESP32S3-RTOS.ino
3. 选择板卡 `ESP32S3 Dev Module
4. 勾选正确的端口
5. 编译上传

### 2. 使用网页串口客户端

方法一：VS Code 打开 `web-serial-client/ 文件夹，使用 **Live Server** 插件启动

方法二：命令行启动本地服务器：
```bash
cd web-serial-client
python3 -m http.server 8000
```

然后浏览器打开：`http://localhost:8000/index.html

**注意**：必须使用 **Chrome / Edge 89+**，且必须通过 HTTP/HTTPS 访问，file:// 不支持 Web Serial API。

### 网页使用步骤：

1. 点击 **选择串口** → 选择你的 ESP32 串口
2. 点击 **连接** → 开始接收数据
3. 如果没有硬件，可以点击 **开始模拟** 生成网页模拟数据

## 📊 支持的传感器

| 参数         | 范围         | 单位 |
|--------------|-------------|------|
| 温度         | 20 - 35     | ℃    |
| 湿度         | 30 - 80     | %RH  |
| 气压         | 980 - 1040  | hPa  |
| CO₂          | 400 - 2000  | ppm  |
| 臭氧 (O₃)    | 0 - 100     | ppb  |
| 乙醛 (C₂H₄O) | 0 - 50      | ppb  |
| 乙烯 (C₂H₄)  | 0 - 5       | ppm  |

## ⚙️ FreeRTOS 任务配置

| 任务       | 栈大小 (字) | 优先级 | 功能 |
|------------|------------|--------|------|
| button     | 2048 (8KB) | 2      | 按键扫描 |
| clock      | 2048 (8KB) |  2      | 时间更新 |
| sensor     | 4096 (16KB)| 3      | 传感器采集 |
| display    | 4096 (16KB)| 3      | 显示更新 |
| ble        | 4096 (16KB)| 4      | BLE 发送 |

所有任务固定运行在 **Core 1**，Arduino 核心运行在 Core 0。

## 📝 开发说明

- 所有延时统一使用 `vTaskDelay(pdMS_TO_TICKS(ms)`，不使用 Arduino `delay()`
- 共享资源使用互斥锁保护（显示资源）
- 任务之间通过队列传递事件/数据
- 完整日志输出，方便调试
- 单个文件代码不超过 500 行，易维护

## 🐛 常见问题

### Q: 上传失败 "Failed to connect to ESP32S3: No serial data received.
A: 这是下载模式问题，解决方法：
1. 按住 BOOT 按键不放 → 按一下 RESET → 松开 BOOT → 再尝试上传

### Q: 网页显示 "First argument to DataView constructor must be an ArrayBuffer
A: 已修复，请拉取最新代码刷新重试

### Q: BLE 连接不上
A: 检查 UUID 是否正确，NimBLE 版本是否兼容

## 📄 网页客户端功能

网页客户端功能：
- 实时接收串口二进制数据
- 显示所有传感器数值
- 统计总包数、错误数、接收速率
- 网页端模拟数据（无硬件测试）
- 滚动日志显示

## 🎯 架构重构说明

本项目原先是单文件堆砌，现在按照功能拆分多文件，遵循以下原则：
1. 单一职责：每个文件只做一件事
2. 统一规范：统一 FreeRTOS API 使用
3. 配置集中：所有配置在 Config.h，修改方便
4. 错误处理：队列满了打日志，不崩溃
5. 可维护：单个文件不超过 500 行

## 许可证

MIT

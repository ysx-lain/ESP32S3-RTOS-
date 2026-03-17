# ESP32S3 FreeRTOS 多任务 BLE 传感器项目

[![Build Status](https://img.shields.io/badge/platform-ESP32S3-blue.svg)](https://www.espressif.com/zh-hans/products/socs/esp32s3)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

基于 Arduino 框架 + FreeRTOS 多任务架构的 ESP32S3 传感器采集项目，支持网页串口客户端，无需编译直接使用。

## 📋 项目简介

本项目演示了如何在 ESP32S3 上使用 FreeRTOS 多任务架构开发传感器采集应用，特点：

- ✅ **规范 FreeRTOS 架构**：每个功能一个独立任务，统一栈大小/优先级配置
- ✅ **多页面显示**：传感器数据 / 运行时间 / 系统状态 三页循环切换
- ✅ **10Hz 采集 + 发送**：传感器每 100ms 更新一次，满足低延迟需求
- ✅ **二进制传输**：直接发送结构体，网页客户端直接解析，效率高
- ✅ **网页串口客户端**：基于 Web Serial API，无需编译，Chrome/Edge 直接打开使用
- ✅ **模拟数据生成**：无硬件时可在网页模拟四种气体（CO₂/臭氧/乙醛/乙烯）变化
- ✅ **自动息屏**：10秒无操作自动关闭背光，按键唤醒，省电
- ✅ **完整日志**：LOG 宏封装，分级日志方便调试
- ✅ **工程化**：PlatformIO 支持，依赖版本锁定，便于复现

## 🔧 硬件接线

| 功能       | ESP32S3 GPIO | 说明                |
|------------|-------------|---------------------|
| 按键       | GPIO2       | 接 GND，内部上拉   |
| 背光       | GPIO12      | 高电平点亮 OLED/ST7735 屏幕 |
| 屏幕 SCLK  | GPIO13      | SPI 时钟            |
| 屏幕 MOSI  | GPIO11      | SPI 数据            |
| 屏幕 CD    | GPIO9       | 命令/数据           |
| 屏幕 CS    | GPIO10      | 片选                |
| 屏幕 RESET | GPIO8       | 复位                |

> 项目中使用的是 Ucglib 库驱动屏幕，适配大部分 SPI 屏幕，如果你使用其他屏幕，修改 `Display.cpp` 即可。

## 📦 环境搭建

### 方法一：Arduino IDE（适合新手）

1. **安装 ESP32 Arduino Core**
   - 文件 → 首选项 → 附加开发板管理器网址：
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - 工具 → 开发板 → 开发板管理器 → 搜索 `esp32` → 安装 `ESP32 by Espressif Systems` 版本 **2.0.11+**

2. **安装依赖库**
   - 项目 → 加载库 → 库管理器 → 搜索安装：
     - `Ucglib` → by oliver-olic
     - `NimBLE-Arduino` → by h2zero

3. **下载项目**
   ```bash
   git clone https://github.com/ysx-lain/ESP32S3-RTOS-.git
   ```

4. **打开项目**
   - Arduino IDE → 文件 → 打开 → 选择项目文件夹中的 `ESP32S3-RTOS.ino`

5. **配置板卡**
   - 工具 → 开发板 → 选择 `ESP32S3 Dev Module`
   - 工具 → 上传速度 → 选择 `921600`
   - *（可选）* 工具 → 核心调试级别 → 选择 `Open Debug Level: Core Debug Level: Error` 开启栈溢出检测

### 方法二：PlatformIO（推荐，工程化）

1. 安装 [PlatformIO IDE](https://platformio.org/install/ide)（VS Code 插件）

2. 克隆项目：
   ```bash
   git clone https://github.com/ysx-lain/ESP32S3-RTOS-.git
   cd ESP32S3-RTOS-
   ```

3. VS Code 打开文件夹，PlatformIO 会自动安装依赖：
   - 平台版本锁定：`espressif32@6.4.0`
   - 库版本锁定：`Ucglib@^1.0.6`，`NimBLE-Arduino@^1.4.0`

4. 点击底部 → 编译 → 上传

## 🚀 编译上传

### Arduino IDE

1. 工具 → 端口 → 选择你的 ESP32S3 串口
2. 点击 → 上传按钮
3. 上传失败看 [常见问题](#-常见问题)

### PlatformIO

```bash
# 编译
pio run

# 编译 + 上传
pio run --target upload

# 打开串口监视器
pio device monitor -b 115200
```

## 📁 项目结构

```
ESP32S3-RTOS/
├── ESP32S3-RTOS.ino  ← 主入口，仅放初始化，代码精简
├── platformio.ini     ← PlatformIO 配置（依赖版本锁定）
├── Config.h          ← 全局配置（引脚、栈大小、优先级、范围、日志宏）
├── TaskInit.h/cpp    ← 统一创建所有任务、互斥锁、队列
├── ButtonTask.h/cpp  ← 按键扫描任务（软件消抖 + 事件队列）
├── ClockTask.h/cpp   ← 时钟/运行时间任务
├── SensorTask.h/cpp  ← 传感器采集任务（四种气体模拟数据生成）
├── DisplayTask.h/cpp ← 显示更新任务（三页切换 + 自动息屏）
├── BLETask.h/cpp     ← BLE 通信任务（二进制数据发送）
├── ble.h/cpp         ← BLE 模块封装（基于 NimBLE）
├── Display.h/cpp     ← 屏幕驱动封装（基于 Ucglib）
├── web-serial-client/ ← 网页串口客户端（无需编译）
│   └── index.html     ← HTML + JavaScript 实现
├── .github/
│   ├── ISSUE_TEMPLATE.md ← Issue 模板
│   └── PULL_REQUEST_TEMPLATE.md ← PR 模板
├── LICENSE           ← MIT 开源协议
└── README.md         ← 本文档
```

### 设计原则

- **单一职责**：每个任务一个文件，每个文件只做一件事
- **配置集中**：所有可配置项在 `Config.h`，修改方便
- **规范 API**：所有延时统一用 `vTaskDelay(pdMS_TO_TICKS())`，不使用 `delay()`
- **错误处理**：所有初始化都检查返回值，失败打印日志
- **大小控制**：单个文件代码不超过 500 行，易维护
- **日志分级**：INFO/WARN/ERROR 分级日志，可编译关闭

## 🎮 使用方法

### 硬件端

1. 上电后，ESP32 初始化完成，开始：
   - 按键扫描：按下按键切换页面 / 唤醒屏幕
   - 传感器采集：10Hz 更新模拟数据
   - BLE 广播等待连接
   - 显示第一页传感器数据

2. BLE 连接成功后，自动发送二进制数据

### 网页串口客户端

当 ESP32 通过 USB 连接电脑后，可以在网页上查看实时数据：

**方法一：VS Code Live Server**
- 打开 `web-serial-client/` 文件夹
- 右键 `index.html` → 选择 "Open with Live Server"

**方法二：Python 本地服务器**
```bash
cd web-serial-client
python3 -m http.server 8000
```
然后浏览器打开：`http://localhost:8000/index.html`

> ⚠️ **注意**：必须使用 **Chrome / Edge 版本 89+**，且必须通过 HTTP/HTTPS 访问，`file://` 协议不支持 Web Serial API。

### 网页使用步骤

1. 点击 **选择串口** → 弹出系统对话框，选择你的 ESP32 串口
2. 点击 **连接** → 开始接收数据
3. 如果**没有硬件**，点击 **开始模拟** → 网页生成模拟数据，可以直接测试显示

## 📊 支持的传感器参数

| 参数         | 范围         | 单位 | 说明                |
|--------------|-------------|------|---------------------|
| 温度         | 20 - 35     | ℃    | 环境温度            |
| 湿度         | 30 - 80     | %RH  | 相对湿度            |
| 气压         | 980 - 1040  | hPa  | 大气压强            |
| CO₂          | 400 - 2000  | ppm  | 二氧化碳浓度        |
| 臭氧 (O₃)    | 0 - 100     | ppb  | 臭氧浓度            |
| 乙醛 (C₂H₄O) | 0 - 50      | ppb  | 乙醛浓度            |
| 乙烯 (C₂H₄)  | 0 - 5       | ppm  | 乙烯浓度            |

> 当前使用随机游走算法生成模拟数据，如果你接了真实传感器，替换 `SensorTask.cpp` 中的采集代码即可，框架已经做好了。

## 📡 串口通信协议

ESP32 通过 USB 串口发送**二进制数据包**，网页客户端直接解析。

### 数据包格式

| 偏移 (字节) | 名称         | 类型    | 字节数 | 说明                |
|-------------|--------------|---------|--------|---------------------|
| 0           | temperature  | float   | 4      | 温度 ℃              |
| 4           | humidity     | float   | 4      | 湿度 %RH             |
| 8           | count        | int32   | 4      | 数据包计数          |
| 12          | pressure     | float   | 4      | 气压 hPa             |
| 16          | co2          | int32   | 4      | CO₂ 浓度 ppm         |
| 20          | ozone        | float   | 4      | 臭氧 ppb             |
| 24          | acetaldehyde | float   | 4      | 乙醛 ppb             |
| 28          | ethylene     | float   | 4      | 乙烯 ppm             |
| 32          | timestamp    | uint32  | 4      | ESP32 时间戳 (ms)    |

**总大小：36 字节**

### 字节序

- 小端序 (little-endian)，和 x86/ARM 一致，JavaScript `DataView` 直接解析

### 波特率

- 串口波特率：`115200`
- 数据位：8，停止位：1，无校验

### 发送频率

- 默认 10Hz (每 100ms 一包)，可在 `Config.h` 修改 `INTERVAL_BLE`

## ⚙️ FreeRTOS 任务配置

所有任务固定运行在 **Core 1**，Arduino 核心运行在 Core 0，避免相互阻塞。

| 任务       | 栈大小 (字) | 栈大小 (字节) | 优先级 | 功能                 |
|------------|------------|-------------|--------|----------------------|
| button     | 2048       | 8KB         | 2      | 按键扫描、消抖       |
| clock      | 2048       | 8KB         | 2      | 运行时间维护         |
| sensor     | 4096       | 16KB        | 3      | 传感器采集、模拟生成 |
| display    | 4096       | 16KB        | 3      | 显示更新、页面切换   |
| ble        | 4096       | 16KB        | 4      | BLE 数据发送         |

优先级规则：**通信 > 采集 > IO**，保证关键任务不被阻塞。

## 🔗 BLE 通信协议

ESP32 作为 BLE GATT Server，使用 Notify 方式发送传感器数据给客户端。

### BLE UUID

| 类型 | UUID |
|------|------|
| Service | `4fafc201-1fb5-459e-8fcc-c5c9c331914b` |
| TX Characteristic (Notify) | `1b9a473a-4493-4536-8b2b-9d4133488256` |
| RX Characteristic (Write) | `2c9b473a-5594-4736-9b2b-8d5234489357` |

### 数据包格式

和串口一致，也是 36 字节二进制数据包，格式同上 **[串口通信协议](#-串口通信协议)**。

### 通信流程

1. ESP32 上电后开始广播
2. BLE 客户端连接
3. 客户端使能 TX Characteristic Notify
4. ESP32 有新传感器数据时自动发送 Notify
5. 发送频率：10Hz，和串口一致

## 🌟 实验现象

1. **上电** → 屏幕点亮，显示第一页传感器数据
   - 可以看到所有 7 个参数缓慢变化
2. **按按键** → 切换页面：
   - 第 1 页：传感器数据
   - 第 2 页：系统运行时间
   - 第 3 页：BLE 连接状态、运行时间、页码
3. **10 秒无操作** → 屏幕自动熄灭（背光关闭）
4. **按按键** → 屏幕唤醒，回到之前页面
5. **BLE / 串口连接成功** → ESP32 每 100ms 发送一包二进制数据
6. **网页客户端** → 实时显示所有参数，显示统计信息（总包数、错误数、速率）

## 🐛 常见问题

### Q: 上传失败 `A fatal error occurred: Failed to connect to ESP32-S3: No serial data received.`
A: 这是下载模式问题，解决方法：
1. 按住 ESP32S3 板上 **BOOT** 按键不放
2. 按一下 **RESET** 按键
3. 松开 **BOOT** 按键
4. 重新点击上传

### Q: 网页显示 `First argument to DataView constructor must be an ArrayBuffer`
A: 已修复，请拉取最新代码刷新浏览器重试。问题原因是二进制解析不正确，现在已经彻底修复。

### Q: BLE 连接不上
A: 检查：
1. BLE 服务 UUID 是否正确：`4fafc201-1fb5-459e-8fcc-c5c9c331914b`
2. NimBLE-Arduino 版本是否正确，推荐 1.4.0+
3. 设备名称是 `ESP32-S3_Sensor`

### Q: 编译报错 `multiple definition of ...`
A: 确保所有 `.cpp` 和 `.h` 都在 Arduino IDE 项目文件夹的**根目录**，Arduino IDE 会编译目录下所有 `.cpp` 文件，不要放子文件夹里。本项目已经放在根目录了，直接打开即可。

### Q: 提示 `library Ucglib claims to run on avr, sam architecture(s) and may be incompatible`
A: 警告可以忽略，实际 ESP32 可以正常编译使用。

## 👥 参与贡献

欢迎提交 Issue 和 Pull Request！

- 如果你发现 Bug，请提交 [Issue](../../issues/new) 并附上错误日志
- 如果你想贡献代码，请提交 [PR](../../pulls)
- 改进建议也欢迎提出

### Issue 模板

项目提供了 Issue 模板，新建 Issue 时会自动加载，请按照模板填写。

## 📄 开源协议

[MIT License](LICENSE) © 2026 ysx

你可以自由使用、修改、分发本项目，用于个人和商业用途。

## ✨ 致谢

- [Ucglib](https://github.com/olikraus/ucglib) - 优秀的单色 LCD 图形库
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) - 轻量 BLE 库
- [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API) - 浏览器串口 API

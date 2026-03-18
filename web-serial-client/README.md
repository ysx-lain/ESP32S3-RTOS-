# ESP32S3 传感器数据 Web 串口客户端

## 功能

基于浏览器 **Web Serial API** 实现，无需编译，直接打开就能用！

- ✅ 直接在浏览器中打开使用
- ✅ 自动搜索串口，选择连接
- ✅ 实时显示传感器数据
- ✅ 统计总包数、错误数、接收速率
- ✅ 漂亮的UI，自适应布局

## 使用方法

1. ** requirements **
   - Chrome/Edge 版本 89+（支持 Web Serial API）
   - ESP32S3 通过 USB 连接电脑

2. **打开**
   - 直接双击 `index.html` 在浏览器打开，或者用 VS Code Live Server 打开
   - 或者部署到任何静态网页服务器

3. **连接**
   - 点击"刷新端口"，选择你的 ESP32 串口
   - 点击"连接"，开始接收数据

## 数据格式

期待 ESP32 发送二进制结构体，格式：

| 字段 | 类型 | 字节 |
|------|------|------|
| temperature | float | 4 |
| humidity | float | 4 |
| count | int32 | 4 |
| pressure | float | 4 |
| co2 | int32 | 4 |
| timestamp | uint32 | 4 |
| **总计** | | **24 字节** |

和 ESP32 端 `SensorReading_t` 结构体完全匹配。

## 优势

相比 Windows 桌面 exe：
- 不需要编译
- 不需要安装 .NET
- 直接打开使用
- 跨平台，macOS/Linux/Windows 都能用
- 浏览器打开就行


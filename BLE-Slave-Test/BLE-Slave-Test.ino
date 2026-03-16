/**
 * @file BLE-Slave-Test.ino
 * @brief BLE 从机测试程序：接收主机发来的传感器数据，通过串口输出
 * 
 * 功能：
 *  - 作为 BLE 客户端连接到主机（ESP32S3-RTOS 手表）
 *  - 接收带时间戳的传感器数据
 *  - 验证数据同步性
 *  - 通过串口输出接收数据和时间戳延迟
 * 
 * 硬件：ESP32-S3
 * 作者：ysx
 * 日期：2024-03-16
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Arduino.h>

// ==================== 配置 ====================
// 要连接的设备名称
#define TARGET_DEVICE_NAME "ESP32-S3_Sensor"
// 服务 UUID 和特征 UUID（要和主机一致，和 BLEManager 匹配）
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "1b9a473a-4493-4536-8b2b-9d4133488256"

// ==================== 全局变量 ====================
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
bool deviceConnected = false;
bool gotData = false;

// 保存找到的目标设备
BLEScan* pBLEScan;
BLEAdvertisedDevice* targetDevice = nullptr;
bool foundTarget = false;

// 统计信息
unsigned long totalPackets = 0;
unsigned long totalErrors = 0;
unsigned long lastPacketTime = 0;
unsigned long minLatency = 9999;
unsigned long maxLatency = 0;
unsigned long sumLatency = 0;

// ==================== 接收的数据结构 ====================
typedef struct {
    float temperature;      // 温度
    float humidity;          // 湿度
    int count;              // 计数
    float pressure;         // 气压
    int co2;                // CO2
    unsigned long timestamp; // 主机时间戳（毫秒）
} SensorReading;

SensorReading lastReading;

// ==================== 回调类 ====================
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.printf("Found device: %s\n", advertisedDevice.getName().c_str());
        if (advertisedDevice.getName() == TARGET_DEVICE_NAME) {
            Serial.println("✅ Found target device: " + String(TARGET_DEVICE_NAME));
            foundTarget = true;
            // 保存设备地址
            targetDevice = new BLEAdvertisedDevice(advertisedDevice);
            // 停止扫描
            BLEDevice::getScan()->stop();
        }
    }
};

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        deviceConnected = true;
        Serial.println("=== Connected to master device ===");
    }

    void onDisconnect(BLEClient* pclient) {
        deviceConnected = false;
        foundTarget = false;
        targetDevice = nullptr;
        Serial.println("*** Disconnected from master device ***");
        pClient = nullptr;
    }
};

// ==================== 数据接收回调 ====================
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

    if (length != sizeof(SensorReading)) {
        Serial.printf("❌ Error: wrong data length %d, expected %d\n", 
                   (int)length, (int)sizeof(SensorReading));
        totalErrors++;
        return;
    }

    // 复制数据
    memcpy(&lastReading, pData, sizeof(SensorReading));

    // 计算延迟（本地时间 - 主机时间戳）
    unsigned long now = millis();
    unsigned long latency = now - lastReading.timestamp;

    // 更新统计
    totalPackets++;
    sumLatency += latency;
    if (latency < minLatency) minLatency = latency;
    if (latency > maxLatency) maxLatency = latency;
    lastPacketTime = now;

    // 输出到串口
    Serial.printf("\n=== Received sensor data ===\n");
    Serial.printf("Temperature: %.1f C\n", lastReading.temperature);
    Serial.printf("Humidity:    %.1f %%\n", lastReading.humidity);
    Serial.printf("Pressure:    %.2f hPa\n", lastReading.pressure);
    Serial.printf("CO2:         %d ppm\n", lastReading.co2);
    Serial.printf("Count:       %d\n", lastReading.count);
    Serial.printf("Timestamp:   %lu ms (latency: %lu ms)\n", 
               lastReading.timestamp, latency);

    // 每 10 个包输出一次统计
    if (totalPackets % 10 == 0) {
        unsigned long avgLatency = sumLatency / totalPackets;
        Serial.printf("\n--- Statistics ---\n");
        Serial.printf("Total packets: %lu\n", totalPackets);
        Serial.printf("Errors:        %lu\n", totalErrors);
        Serial.printf("Latency min:   %lu ms\n", minLatency);
        Serial.printf("Latency max:   %lu ms\n", maxLatency);
        Serial.printf("Latency avg:   %lu ms\n", avgLatency);
        Serial.printf("------------------\n");
    }

    gotData = true;
}

// ==================== 连接函数 ====================
bool connectToServer() {
    if (!foundTarget || targetDevice == nullptr) {
        Serial.println("🔍 Scanning for target device...");
        foundTarget = false;
        pBLEScan->clearResults();
        pBLEScan->start(5, false); // 扫描 5 秒
        return false; // 扫描完会找到设备，下次循环连接
    }

    // 已经找到目标设备，开始连接
    Serial.println("🔗 Connecting to " + String(TARGET_DEVICE_NAME) + "...");

    // 创建客户端
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    // 连接到设备
    if (!pClient->connect(targetDevice)) {
        Serial.println("❌ Connection failed");
        delete targetDevice;
        targetDevice = nullptr;
        foundTarget = false;
        return false;
    }

    // 找到服务和特征
    Serial.println("🔍 Looking for service...");
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == nullptr) {
        Serial.println("❌ Can't find our service");
        pClient->disconnect();
        delete targetDevice;
        targetDevice = nullptr;
        foundTarget = false;
        return false;
    }

    Serial.println("🔍 Looking for characteristic...");
    pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("❌ Can't find our characteristic");
        pClient->disconnect();
        delete targetDevice;
        targetDevice = nullptr;
        foundTarget = false;
        return false;
    }

    // 注册通知
    if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
        Serial.println("✅ Registered for notifications");
    }

    return true;
}

// ==================== 初始化 ====================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== BLE Slave Test Starting ===");
    Serial.printf("Target device name: %s\n", TARGET_DEVICE_NAME);
    Serial.printf("Expected data size: %d bytes\n", (int)sizeof(SensorReading));

    // 初始化 BLE
    BLEDevice::init("ESP32S3-BLE-Slave-Test");

    // 开始扫描
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->setActiveScan(true);
    ::pBLEScan = pBLEScan; // 保存全局指针

    Serial.println("🔍 Starting scan...");
}

// ==================== 主循环 ====================
// 在 ESP32 Arduino 中，loop 就是一个 FreeRTOS 任务
// 这里只需要处理 BLE 连接维护
void loop() {
    if (!deviceConnected) {
        // 未连接，尝试重连
        if (!connectToServer()) {
            if (!foundTarget) {
                Serial.println("❌ Target not found, retrying in 5 seconds");
            }
            delay(5000);
        } else {
            Serial.println("✅ Connected successfully! Waiting for data...");
        }
    } else {
        // 已连接，检查连接状态
        if (pClient != nullptr && !pClient->isConnected()) {
            deviceConnected = false;
            Serial.println("❌ Connection lost");
        }
    }

    delay(1000);
}

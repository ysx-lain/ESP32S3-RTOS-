/**
 * @file ble.cpp
 * @brief BLE 通讯模块实现文件(基于 NimBLE 库)
 * 
 * 本文件实现了 BLEManager 类的所有方法, 包括初始化, 回调处理, 数据发送等. 
 * 
 * @note 已移除 override 关键字, 以兼容当前 NimBLE 库版本. 
 * 
 * @author 根据用户原始代码适配
 * @date 2024-03-02
 * @version 1.1
 */

#include "ble.h"

// 服务与特征的 UUID
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TX "1b9a473a-4493-4536-8b2b-9d4133488256"
#define CHARACTERISTIC_UUID_RX "2c9b473a-5594-4736-9b2b-8d5234489357"

// 静态成员定义
BLEManager* BLEManager::_instance = nullptr;
NimBLEServer* BLEManager::_pServer = nullptr;
NimBLECharacteristic* BLEManager::_pTxCharacteristic = nullptr;
NimBLECharacteristic* BLEManager::_pRxCharacteristic = nullptr;
bool BLEManager::_deviceConnected = false;
bool BLEManager::_oldDeviceConnected = false;
unsigned long BLEManager::_lastSendTime = 0;
const unsigned long BLEManager::SEND_INTERVAL = 100; // 100ms 最小间隔
void (*BLEManager::_receiveCallback)(const String&) = nullptr;

// ==================== 回调实现 ====================

/**
 * @brief 连接成功回调
 * 
 * @param pServer 触发事件的服务器指针
 */
void BLEManager::ServerCallbacks::onConnect(NimBLEServer* pServer) {
    BLEManager::_deviceConnected = true;
    Serial.println("[BLE] Device connected");
}

/**
 * @brief 断开连接回调
 * 
 * @param pServer 触发事件的服务器指针
 */
void BLEManager::ServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    BLEManager::_deviceConnected = false;
    Serial.println("[BLE] Device disconnected");
}

/**
 * @brief 特征写入回调(客户端发送数据到设备)
 * 
 * @param pCharacteristic 被写入的特征指针
 */
void BLEManager::CharCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        Serial.print("[BLE] Received: ");
        Serial.println(value.c_str());
        if (BLEManager::_receiveCallback) {
            BLEManager::_receiveCallback(String(value.c_str()));
        }
    }
}

/**
 * @brief 特征读取回调(客户端读取特征时触发)
 * 
 * @param pCharacteristic 被读取的特征指针
 */
void BLEManager::CharCallbacks::onRead(NimBLECharacteristic* pCharacteristic) {
    // 可留空
}

// ==================== 公开方法 ====================

/**
 * @brief 初始化 BLE 服务并开始广播
 * 
 * @param deviceName 广播的设备名称
 */
void BLEManager::begin(const char* deviceName) {
    Serial.println("[BLE] Initializing...");

    // 初始化 NimBLE 协议栈
    NimBLEDevice::init(deviceName);

    // 创建服务器
    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(new ServerCallbacks());

    // 创建服务
    NimBLEService* pService = _pServer->createService(SERVICE_UUID);

    // 创建发送特征(Notify)
    _pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY
    );

    // 为发送特征添加 CCCD 描述符(UUID = 0x2902)
    NimBLEDescriptor* pDesc = new NimBLEDescriptor(
        NimBLEUUID((uint16_t)0x2902),                    // CCCD UUID
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, // 属性:可读可写
        2,                                                // 最大长度 2 字节
        nullptr                                           // 所属特征(可选)
    );
    _pTxCharacteristic->addDescriptor(pDesc);

    // 创建接收特征(Write)
    _pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE
    );
    _pRxCharacteristic->setCallbacks(new CharCallbacks());

    // 启动服务
    pService->start();

    // 配置广播
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    
    // 在新版 NimBLE 中，设备名称放在扫描响应中
    NimBLEAdvertisementData advertisementData;
    advertisementData.setCompleteServices(NimBLEUUID(SERVICE_UUID));
    advertisementData.setFlags();
    pAdvertising->setAdvertisementData(advertisementData);
    
    NimBLEAdvertisementData scanResponseData;
    scanResponseData.setName(deviceName);  // 将设备名称放在扫描响应中
    pAdvertising->setScanResponseData(scanResponseData);

    // 开始广播
    pAdvertising->start();

    Serial.println("[BLE] Advertising started");
}

/**
 * @brief 发送传感器数据(字符串形式)
 * 
 * @param data 要发送的字符串
 * @return true  发送成功
 * @return false 发送失败
 */
bool BLEManager::sendSensorData(const String& data) {
    if (!_deviceConnected) return false;
    unsigned long now = millis();
    if (now - _lastSendTime < SEND_INTERVAL) return false;
    _pTxCharacteristic->setValue(data.c_str());
    _pTxCharacteristic->notify();
    _lastSendTime = now;
    return true;
}

/**
 * @brief 发送传感器数据(字节数组形式)
 * 
 * @param data   数据指针
 * @param length 数据长度
 * @return true  发送成功
 * @return false 发送失败
 */
bool BLEManager::sendSensorData(uint8_t* data, size_t length) {
    if (!_deviceConnected) return false;
    unsigned long now = millis();
    if (now - _lastSendTime < SEND_INTERVAL) return false;
    _pTxCharacteristic->setValue(data, length);
    _pTxCharacteristic->notify();
    _lastSendTime = now;
    return true;
}

/**
 * @brief 维护 BLE 状态, 处理断开后自动重广播
 */
void BLEManager::update() {
    // 处理断开后重新广播
    if (!_deviceConnected && _oldDeviceConnected) {
        delay(500);
        NimBLEDevice::getAdvertising()->start();
        _oldDeviceConnected = _deviceConnected;
        Serial.println("[BLE] Restarted advertising");
    }
    if (_deviceConnected != _oldDeviceConnected) {
        _oldDeviceConnected = _deviceConnected;
        // 连接状态变化时打印日志
        if (_deviceConnected) {
            Serial.println("[BLE] Device connected");
        } else {
            Serial.println("[BLE] Device disconnected");
        }
    }
}

/**
 * @brief 注册接收数据的回调函数
 * 
 * @param callback 用户函数指针, 参数为接收到的字符串
 */
void BLEManager::onReceiveData(void (*callback)(const String&)) {
    _receiveCallback = callback;
}
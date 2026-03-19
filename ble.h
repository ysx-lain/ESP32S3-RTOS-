/**
 * @file ble.h
 * @brief BLE 通讯模块头文件(基于 NimBLE 库)
 * 
 * 本文件声明了 BLEManager 类, 用于管理蓝牙低功耗服务. 
 * 包含服务器, 特征, 回调的声明, 以及对外接口. 
 * 
 * @note 已移除 override 关键字, 以兼容当前 NimBLE 库版本. 
 * 
 * @author 根据用户原始代码适配
 * @date 2024-03-02
 * @version 1.1
 */

#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>

/**
 * @brief BLE 管理器类(单例模式)
 * 
 * 封装 NimBLE 服务器, 特征及回调. 所有成员均为静态. 
 */
class BLEManager {
private:
    static BLEManager* _instance;               ///< 单例实例指针(当前未使用)
    static NimBLEServer* _pServer;               ///< BLE 服务器对象指针
    static NimBLECharacteristic* _pTxCharacteristic; ///< 发送特征(Notify)
    static NimBLECharacteristic* _pRxCharacteristic; ///< 接收特征(Write)
    static bool _deviceConnected;                ///< 当前是否有设备连接
    static bool _oldDeviceConnected;             ///< 上一次连接状态
    static unsigned long _lastSendTime;          ///< 上次发送数据的时间戳
    static const unsigned long SEND_INTERVAL;    ///< 最小发送间隔(毫秒)

    /**
     * @brief 服务器回调类
     * 
     * 处理连接和断开事件. 
     */
    class ServerCallbacks : public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* pServer);    ///< 连接事件回调
        void onDisconnect(NimBLEServer* pServer); ///< 断开事件回调
    };

    /**
     * @brief 特征回调类
     * 
     * 处理客户端写入/读取特征的事件. 
     */
    class CharCallbacks : public NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic* pCharacteristic); ///< 写入事件回调
        void onRead(NimBLECharacteristic* pCharacteristic);  ///< 读取事件回调
    };

    BLEManager() {} ///< 私有构造函数

public:
    /**
     * @brief 初始化 BLE 服务器并开始广播
     * 
     * @param deviceName 广播的设备名称(默认:"ESP32-S3_Sensor")
     */
    static void begin(const char* deviceName = "ESP32-S3_Sensor");

    /**
     * @brief 发送传感器数据(字符串形式)
     * 
     * @param data 要发送的字符串
     * @return true  发送成功
     * @return false 发送失败
     */
    static bool sendSensorData(const String& data);

    /**
     * @brief 发送传感器数据(字节数组形式)
     * 
     * @param data   数据指针
     * @param length 数据长度
     * @return true  发送成功
     * @return false 发送失败
     */
    static bool sendSensorData(uint8_t* data, size_t length);

    /**
     * @brief 检查当前是否有 BLE 客户端连接
     * 
     * @return true  有设备连接
     * @return false 无设备连接
     */
    static bool isConnected() { return _deviceConnected; }

    /**
     * @brief 维护 BLE 状态(应在主循环中调用)
     */
    static void update();

    /**
     * @brief 注册接收数据的回调函数
     * 
     * @param callback 函数指针, 参数为接收到的字符串
     */
    static void onReceiveData(void (*callback)(const String&));

private:
    static void (*_receiveCallback)(const String&); ///< 用户注册的回调函数指针
};

#endif
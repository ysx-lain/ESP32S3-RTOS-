/**
 * @file main.ino
 * @brief 主程序：集成屏幕驱动、系统时钟、BLE通讯、页面管理、按键控制、自动息屏
 * 
 * 硬件接线：
 *   - 屏幕：sclk=13, data=11, cd=9, cs=10, reset=8
 *   - 背光：GPIO12（高电平点亮）
 *   - 按键：GPIO2（接 GND，内部上拉）
 * 
 * 功能：
 *   - 按键切换页面（传感器页 / 运行时间页）
 *   - 运行时间页每分钟更新一次（分钟变化时才刷新屏幕）
 *   - 10秒无操作自动息屏，按键唤醒
 *   - 唤醒或切换页面时先清屏，确保无内容重叠
 *   - 可选 BLE 广播（默认注释，可取消注释启用）
 * 
 * @author ysx
 * @date 2024-03-02
 * @version 3.3
 */

#include <SPI.h>
#include "Ucglib.h"
#include "Display.h"
#include "clock.h"      // 系统时钟（可选，这里暂不使用其 ticks，改用 millis）
// #include "ble.h"      // BLE 模块（如需启用，取消注释并安装标准BLE库）

// ==================== 硬件引脚定义 ====================
#define BUTTON_PIN 2
#define BL_PIN     12

// ==================== 全局对象 ====================
Display display(13, 11, 9, 10, 8, BL_PIN);

// ==================== 页面管理 ====================
const int PAGE_COUNT = 2;
int currentPage = 0;
unsigned long lastButtonPress = 0;
const unsigned long debounceTime = 200;

typedef void (*PageFunction)();
void page1();  // 传感器页
void page2();  // 运行时间页
PageFunction pages[PAGE_COUNT] = { page1, page2 };

// ==================== 时间显示相关（优化版）====================
// 强制刷新运行时间显示（用于页面切换时立即显示最新时间）
void refreshTimeDisplay() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Uptime: %lud %02lu:%02lu", days, hours % 24, minutes % 60);

    // 中号字体（7x13）的高度为13像素，基线y=50
    const int fontHeight = 13;
    const int baselineY = 50;
    const int clearX = 10;
    const int clearY = baselineY - fontHeight;   // 文本顶部坐标
    const int clearW = 150;                       // 足够宽覆盖整个字符串
    const int clearH = fontHeight + 2;             // 多清除一点以防下降部分残留

    display.fillRect(clearX, clearY, clearW, clearH, 0, 0, 0);
    display.drawText(10, baselineY, buffer, FONT_MEDIUM_SIZE, 255, 255, 255);
}

// 每分钟更新一次（分钟变化才刷新）
void updateTime() {
    if (currentPage != 1) return;      // 只在运行时页面更新

    static int lastMinute = -1;        // 记录上一次显示的分钟值

    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    int currentMinute = minutes % 60;

    // 如果分钟没有变化，则不刷新屏幕（节省资源）
    if (currentMinute == lastMinute) {
        return;
    }
    lastMinute = currentMinute;

    // 分钟已变化，刷新显示
    refreshTimeDisplay();
}

// ==================== 按键处理 ====================
void checkButton() {
    static int lastButtonState = HIGH;
    int buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW && lastButtonState == HIGH && millis() - lastButtonPress > debounceTime) {
        lastButtonPress = millis();
        bool wasScreenOff = !display.isScreenOn();
        display.updateActivity();

        if (wasScreenOff) {
            // 屏幕原来是关的：唤醒并重绘当前页面（先清屏，避免重叠）
            display.clear();
            pages[currentPage]();
            Serial.println("Screen woken up");
        } else {
            // 屏幕亮着：切换页面（先清屏，避免重叠）
            currentPage = (currentPage + 1) % PAGE_COUNT;
            Serial.print("Switched to page: ");
            Serial.println(currentPage);
            display.clear();
            pages[currentPage]();
        }
    }
    lastButtonState = buttonState;
}

// ==================== 页面函数 ====================
void page1() {
    // 传感器页面：直接绘制5行数据（整个屏幕会被覆盖）
    display.Sensor(3, "Temp", 25.5, "C");
    display.Sensor(4, "Hum",  60,   "%");
    display.Sensor(5, "Count", 100, NULL);
    display.Sensor(6, "Press", 1013.25, "hPa");
    display.Sensor(7, "CO2",   400,  "ppm");
}

void page2() {
    // 运行时间页面：先绘制标题，再显示当前时间
    display.drawText(20, 20, "Uptime", FONT_LARGE_SIZE, 0, 255, 255);
    refreshTimeDisplay();   // 立即显示当前时间（内部已处理局部清除）
}

// ==================== BLE 回调（可选）====================
// 如需启用 BLE，取消注释以下部分，并在 setup() 中调用 BLEManager::begin()
/*
void onBLECommand(const String& cmd) {
    Serial.print("BLE command: ");
    Serial.println(cmd);
}
*/

// ==================== 初始化 ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("System starting...");

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // 初始化屏幕
    if (!display.begin(1)) {
        Serial.println("Display init failed!");
        while (1);
    }
    // 移除开机动画，直接显示默认页面
    // display.bootAnimation();   // 已注释，去掉加载界面
    Serial.println("Display ready");

    // 初始化系统时钟（可选，暂不使用其 ticks）
    // SystemClock::begin();

    // 初始化 BLE（如需启用，取消注释）
    // BLEManager::begin("ESP32-S3_Sensor");
    // BLEManager::onReceiveData(onBLECommand);

    // 显示默认页面（页面0，传感器页）
    display.clear();           // 确保屏幕干净
    pages[currentPage]();

    Serial.println("System ready");
}

// ==================== 主循环 ====================
void loop() {
    // 按键检测（唤醒/切换）
    checkButton();

    // 自动息屏检测（10秒无操作息屏）
    display.checkSleep(10000);

    // 维护 BLE 状态（如需启用）
    // BLEManager::update();

    // 每分钟更新运行时间（基于 millis() 轮询）
    static unsigned long lastTimeCheck = 0;
    if (millis() - lastTimeCheck >= 1000) {   // 每秒检查一次
        lastTimeCheck = millis();
        updateTime();                         // 内部判断分钟是否变化
    }

    // 可选的 BLE 数据发送示例（每5秒发送一次）
    /*
    static unsigned long lastBLESend = 0;
    if (BLEManager::isConnected() && millis() - lastBLESend >= 5000) {
        lastBLESend = millis();
        String data = "T:25.5,H:60";
        BLEManager::sendSensorData(data);
        Serial.println("BLE data sent");
    }
    */

    // 短暂延时，避免 WDT 触发
    delay(10);
}
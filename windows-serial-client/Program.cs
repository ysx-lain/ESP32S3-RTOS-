/**
 * @file Program.cs
 * @brief ESP32S3 传感器数据串口接收客户端
 * 
 * 功能：
 *  - 通过串口连接 ESP32S3 主机
 *  - 实时接收传感器数据并显示
 *  - 内置模拟传感器生成功能（测试用，0.1秒生成一次）
 *  - 统计数据接收频率和延迟
 * 
 * 编译：使用 Visual Studio 或者 dotnet publish 生成 exe
 *  dotnet publish -c Release -r win-x64 --self-contained true → 生成独立 exe
 * 
 * @date 2026-03-17
 */

using System;
using System.IO.Ports;
using System.Threading;

namespace Esp32SensorClient
{
    // 传感器数据结构，和 ESP32 一致
    public struct SensorReading
    {
        public float temperature;
        public float humidity;
        public int count;
        public float pressure;
        public int co2;
        public uint timestamp;
    }

    class Program
    {
        private static SerialPort serialPort;
        private static bool running = true;
        private static int totalPackets = 0;
        private static int errorPackets = 0;
        private static DateTime startTime;

        // 模拟传感器参数
        private static float baseTemp = 25.0f;
        private static float baseHum = 50.0f;
        private static float basePress = 1013.0f;
        private static int baseCo2 = 400;
        private static int counter = 0;
        private static Random rand = new Random();

        static void Main(string[] args)
        {
            Console.WriteLine("========================================");
            Console.WriteLine("ESP32S3 BLE 传感器串口客户端");
            Console.WriteLine("========================================");
            Console.WriteLine();

            if (args.Length > 0 && args[0] == "simulate")
            {
                Console.WriteLine("模式：模拟传感器数据生成");
                Console.WriteLine("每秒生成 10 包数据，按 Ctrl+C 停止");
                Console.WriteLine();
                RunSimulation();
                return;
            }

            Console.WriteLine("模式：串口接收 ESP32 传感器数据");
            Console.WriteLine();

            // 列出可用串口
            string[] ports = SerialPort.GetPortNames();
            if (ports.Length == 0)
            {
                Console.WriteLine("没有找到可用串口！");
                Console.ReadKey();
                return;
            }

            Console.WriteLine("可用串口:");
            for (int i = 0; i < ports.Length; i++)
            {
                Console.WriteLine($"  {i + 1}. {ports[i]}");
            }
            Console.WriteLine();
            Console.Write("请选择串口编号 (1-{0}): ", ports.Length);
            
            int selected = int.Parse(Console.ReadLine()) - 1;
            string portName = ports[selected];

            Console.Write("波特率 (默认 115200): ");
            string baudStr = Console.ReadLine();
            int baudRate = string.IsNullOrEmpty(baudStr) ? 115200 : int.Parse(baudStr);

            // 打开串口
            try
            {
                serialPort = new SerialPort(portName, baudRate);
                serialPort.DataReceived += SerialPort_DataReceived;
                serialPort.Open();
                Console.WriteLine();
                Console.WriteLine($"✅ 已打开 {portName}, 波特率 {baudRate}");
                Console.WriteLine("正在接收数据，按 Ctrl+C 退出...");
                Console.WriteLine();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 打开串口失败: {ex.Message}");
                Console.ReadKey();
                return;
            }

            startTime = DateTime.Now;

            // 等待退出
            Console.CancelKeyPress += (sender, e) =>
            {
                running = false;
                e.Cancel = true;
            };

            while (running)
            {
                Thread.Sleep(100);
                // 定期打印统计
                if ((DateTime.Now - startTime).TotalSeconds >= 10 && totalPackets > 0)
                {
                    PrintStats();
                }
            }

            serialPort.Close();
            PrintStats();
            Console.WriteLine();
            Console.WriteLine("按任意键退出...");
            Console.ReadKey();
        }

        private static void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                // 我们接收的是二进制结构体，每次正好 sizeof(SensorReading) 字节
                int bytesToRead = serialPort.BytesToRead;
                int structSize = System.Runtime.InteropServices.Marshal.SizeOf<SensorReading>();

                while (bytesToRead >= structSize)
                {
                    byte[] buffer = new byte[structSize];
                    serialPort.Read(buffer, 0, structSize);
                    SensorReading reading = BytesToStruct(buffer);
                    totalPackets++;

                    // 打印数据
                    TimeSpan age = DateTime.Now - startTime;
                    Console.WriteLine($"[{age.TotalSeconds:F1}s] Temp={reading.temperature:F1}C  Hum={reading.humidity:F1}%  Press={reading.pressure:F1}hPa  CO2={reading.co2}ppm  Count={reading.count}");

                    bytesToRead -= structSize;
                }
            }
            catch (Exception ex)
            {
                errorPackets++;
                Console.WriteLine($"❌ 接收错误: {ex.Message}");
            }
        }

        private static SensorReading BytesToStruct(byte[] bytes)
        {
            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            SensorReading reading = (SensorReading)System.Runtime.InteropServices.Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(SensorReading));
            handle.Free();
            return reading;
        }

        private static void PrintStats()
        {
            double elapsed = (DateTime.Now - startTime).TotalSeconds;
            double pps = totalPackets / elapsed;
            Console.WriteLine();
            Console.WriteLine($"===== 统计 =====");
            Console.WriteLine($"总数据包: {totalPackets}");
            Console.WriteLine($"错误数据包: {errorPackets}");
            Console.WriteLine($"接收速率: {pps:F1} 包/秒");
            Console.WriteLine($"=================");
            Console.WriteLine();
        }

        // ==================== 模拟传感器生成 ====================
        private static void RunSimulation()
        {
            startTime = DateTime.Now;
            Console.CancelKeyPress += (sender, e) =>
            {
                running = false;
                e.Cancel = true;
            };

            Console.WriteLine("时间       温度(℃)  湿度(%)   气压(hPa)  CO2(ppm)  计数");
            Console.WriteLine("----------------------------------------");

            while (running)
            {
                // 随机游走生成模拟数据
                baseTemp += (float)(rand.Next(100) - 50) / 100.0f;
                if (baseTemp < 20) baseTemp = 20;
                if (baseTemp > 35) baseTemp = 35;

                baseHum += (float)(rand.Next(100) - 50) / 10.0f;
                if (baseHum < 30) baseHum = 30;
                if (baseHum > 80) baseHum = 80;

                basePress += (float)(rand.Next(100) - 50) / 10.0f;
                if (basePress < 980) basePress = 980;
                if (basePress > 1040) basePress = 1040;

                baseCo2 += rand.Next(21) - 10;
                if (baseCo2 < 400) baseCo2 = 400;
                if (baseCo2 > 2000) baseCo2 = 2000;

                counter++;

                double elapsed = (DateTime.Now - startTime).TotalSeconds;
                Console.WriteLine($"{elapsed:F1}s  {baseTemp:F1}  {baseHum:F1}  {basePress:F1}  {baseCo2}  {counter}");

                totalPackets++;
                Thread.Sleep(100); // 0.1 秒一次
            }

            PrintStats();
        }
    }
}

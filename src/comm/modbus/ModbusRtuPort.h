// ============================================================
// ModbusRtuPort.h — Modbus RTU（串口）通信端口
// ============================================================
//
// Modbus RTU 是运行在串口（RS-232/RS-485）上的 Modbus 协议，
// 采用二进制帧、紧凑高效，多用于现场仪表与 PLC 的近距离连接。
//
// 本头文件定义：
//   ModbusRtuConfig  RTU 专属配置结构体（串口号、波特率等）
//   ModbusRtuPort    RTU 通信端口类（实现 ICommunicationPort 接口）
//
// 底层依赖 Qt 的 QModbusRtuSerialMaster（Qt6::SerialBus），
// 由它完成 RTU 帧的组帧、校验与收发。
// ============================================================

#pragma once

#include "ICommunicationPort.h"     // 通信端口抽象接口
#include <QModbusRtuSerialMaster>   // Qt 提供的 Modbus RTU 主站类
#include <QModbusClient>            // Qt 提供的 Modbus 客户端基类
#include <memory>                   // std::unique_ptr 智能指针

// ------------------------------------------------------------
// ModbusRtuConfig — Modbus RTU 配置结构体
// ------------------------------------------------------------
// 继承自 DeviceConfig，在其通用字段（名称/协议/轮询间隔等）之上，
// 追加 RTU 串口通信所需的物理层参数。
// ------------------------------------------------------------
struct ModbusRtuConfig : public DeviceConfig
{
    std::string port_name = "COM1";     // 串口名称（Windows 下如 COM1）
    int baudrate = 19200;               // 波特率（每秒传输的符号数）
    char parity = 'N';                  // 校验位：'N' = 无校验, 'E' = 偶校验, 'O' = 奇校验
    int data_bits = 8;                  // 数据位（每个字符的位数）
    int stop_bits = 1;                  // 停止位（帧结束标志位数）
};

// ------------------------------------------------------------
// ModbusRtuPort — Modbus RTU 通信端口类
// ------------------------------------------------------------
// 实现 ICommunicationPort 接口，把串口上的 Modbus RTU 从站
// 抽象为统一的“设备端口”，供通信管理器统一调度。
// ------------------------------------------------------------
class ModbusRtuPort : public ICommunicationPort
{
public:
    ModbusRtuPort();                 // 构造函数
    ~ModbusRtuPort() override;       // 析构函数（断开连接并释放资源）

    bool connect(const DeviceConfig& config) override;   // 连接设备
    void disconnect() override;                          // 断开设备
    bool isConnected() const override;                   // 是否已连接
    DeviceStatus status() const override;                // 获取连接状态
    bool readTag(const std::string& tag_name, double& value) override;  // 读单标签
    bool readTags(const std::vector<std::string>& tags,
        std::vector<double>& values) override;           // 读多标签
    bool writeTag(const std::string& tag_name, double value) override;  // 写单标签
    std::string protocolName() const override;           // 获取协议名称
    bool poll() override;                                // 执行一次轮询

private:
    // 配置串口参数：
    //   将 ModbusRtuConfig 中的串口号、波特率、校验位、数据位、
    //   停止位、超时、重试次数写入底层 QModbus 主站对象。
    void configureSerialPort(const ModbusRtuConfig& cfg);

    std::unique_ptr<QModbusRtuSerialMaster> m_modbus;   // Modbus RTU 主站对象（底层收发）
    ModbusRtuConfig m_config;                           // Modbus RTU 配置
    DeviceStatus m_status = DeviceStatus::DISCONNECTED; // 当前设备连接状态
    int m_fail_count = 0;                               // 失败计数，用于重试机制
};

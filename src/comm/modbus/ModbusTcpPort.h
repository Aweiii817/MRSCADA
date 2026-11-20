// ============================================================
// ModbusTcpPort.h — Modbus TCP（以太网）通信端口
// ============================================================
//
// Modbus TCP 是运行在以太网（TCP/IP）上的 Modbus 协议，
// 通过标准 TCP 连接承载 Modbus 报文，多用于与远程 PLC、网关
// 或带网口的设备通信（默认端口 502）。
//
// 本头文件定义：
//   ModbusTcpConfig  TCP 专属配置结构体（主机地址、端口）
//   ModbusTcpPort    TCP 通信端口类（实现 ICommunicationPort 接口）
//
// 底层依赖 Qt 的 QModbusTcpClient（Qt6::SerialBus），
// 由它完成 TCP 连接与 Modbus 报文的收发。
// ============================================================

#pragma once

#include "ICommunicationPort.h"   // 通信端口抽象接口
#include <QModbusTcpClient>       // Qt 提供的 Modbus TCP 客户端类
#include <memory>                 // std::unique_ptr 智能指针

// ------------------------------------------------------------
// ModbusTcpConfig — Modbus TCP 配置结构体
// ------------------------------------------------------------
// 继承自 DeviceConfig，在其通用字段之上追加 TCP 连接所需的
// 主机地址与端口号。
// ------------------------------------------------------------
struct ModbusTcpConfig : public DeviceConfig
{
    std::string host = "127.0.0.1";   // 主机地址（IP 或主机名）
    int port = 502;                   // 端口号（Modbus TCP 默认 502）
};

// ------------------------------------------------------------
// ModbusTcpPort — Modbus TCP 通信端口类
// ------------------------------------------------------------
// 实现 ICommunicationPort 接口，把以太网上的 Modbus TCP 从站
// 抽象为统一的“设备端口”，供通信管理器统一调度。
// ------------------------------------------------------------
class ModbusTcpPort : public ICommunicationPort
{
public:
    ModbusTcpPort();                 // 构造函数
    ~ModbusTcpPort() override;       // 析构函数（断开连接并释放资源）

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
    std::unique_ptr<QModbusTcpClient> m_modbus;          // Modbus TCP 客户端对象（底层收发）
    ModbusTcpConfig m_config;                            // Modbus TCP 配置
    DeviceStatus m_status = DeviceStatus::DISCONNECTED;  // 当前设备连接状态
    int m_fail_count = 0;                                // 失败计数，用于重试机制
};

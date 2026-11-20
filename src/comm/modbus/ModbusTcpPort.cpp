// ============================================================
// ModbusTcpPort.cpp — Modbus TCP 端口实现
// ============================================================
//
// 实现 ModbusTcpPort 的连接、断开、状态查询与读写接口。
// 底层使用 Qt 的 QModbusTcpClient 完成 TCP 连接与 Modbus
// 报文的收发。
// ============================================================

#include "ModbusTcpPort.h"   // Modbus TCP 端口类声明
#include "Logger.h"          // 日志工具
#include <QVariant>          // QVariant（连接参数值的统一类型）

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 此处不创建底层对象；实际的 QModbusTcpClient 在 connect()
// 中按需创建，保证每次连接都使用最新配置。
// ------------------------------------------------------------
ModbusTcpPort::ModbusTcpPort()
{
}

// ------------------------------------------------------------
// 析构函数
// ------------------------------------------------------------
// 释放前先断开连接，确保 TCP 连接被正确关闭。
// ------------------------------------------------------------
ModbusTcpPort::~ModbusTcpPort()
{
    disconnect();
}

// ------------------------------------------------------------
// connect() — 连接 Modbus TCP 设备
// ------------------------------------------------------------
// 流程：
//   1. 若已连接则先断开，避免重复连接；
//   2. 将传入的通用配置转换为 TCP 配置（dynamic_cast 判断）；
//   3. 创建 QModbusTcpClient 客户端并设置主机、端口、超时与重试；
//   4. 尝试连接，据结果更新状态并记录日志。
// 参数：
//   config — 设备通信配置（实际应为 ModbusTcpConfig 的派生对象）。
// 返回：
//   true  — 连接成功；false — 连接失败。
// ------------------------------------------------------------
bool ModbusTcpPort::connect(const DeviceConfig& config)
{
    if (m_modbus && m_modbus->state() == QModbusDevice::ConnectedState) {
        disconnect();
    }

    // 将通用配置转换为 Modbus TCP 配置（动态类型判断）
    const auto* tcp_config = dynamic_cast<const ModbusTcpConfig*>(&config);
    if (tcp_config) m_config = *tcp_config;

    // 创建 QModbusTcpClient 实例
    m_modbus = std::make_unique<QModbusTcpClient>();

    // 设置连接参数：端口和地址
    m_modbus->setConnectionParameter(
        QModbusDevice::NetworkPortParameter, m_config.port);
    m_modbus->setConnectionParameter(
        QModbusDevice::NetworkAddressParameter,
        QString::fromStdString(m_config.host));

    // 设置超时时间和重试次数
    m_modbus->setTimeout(m_config.timeout_ms);
    m_modbus->setNumberOfRetries(m_config.max_retries - 1);

    // 尝试连接设备
    if (m_modbus->connectDevice()) {
        m_status = DeviceStatus::CONNECTED;
        LOG_INFO("ModbusTCP: connected to {}:{}",
            m_config.host, m_config.port);
        return true;
    }
    else {
        m_status = DeviceStatus::FAULT;
        LOG_ERROR("ModbusTCP: connection failed to {}:{} - {}",
            m_config.host, m_config.port,
            m_modbus->errorString().toStdString());
        return false;
    }
}

// ------------------------------------------------------------
// disconnect() — 断开 Modbus TCP 设备连接
// ------------------------------------------------------------
// 断开底层连接并释放客户端对象，状态置为“未连接”。
// ------------------------------------------------------------
void ModbusTcpPort::disconnect()
{
    if (m_modbus) {
        m_modbus->disconnectDevice();   // 断开底层设备
        m_modbus.reset();               // 释放客户端对象
    }
    m_status = DeviceStatus::DISCONNECTED;
    LOG_INFO("ModbusTCP: disconnected");
}

// ------------------------------------------------------------
// isConnected() — 检查 Modbus TCP 设备是否已连接
// ------------------------------------------------------------
// 返回：
//   true  — 客户端对象存在且处于已连接状态；
//   false — 未创建客户端或未连接。
// ------------------------------------------------------------
bool ModbusTcpPort::isConnected() const
{
    return m_modbus &&
        m_modbus->state() == QModbusDevice::ConnectedState;
}

// ------------------------------------------------------------
// status() — 获取 Modbus TCP 设备连接状态
// ------------------------------------------------------------
DeviceStatus ModbusTcpPort::status() const
{
    return m_status;
}

// ------------------------------------------------------------
// readTag() — 读取单个标签值
// ------------------------------------------------------------
// 目前尚未实现完整的“位号 → Modbus 地址 → 读请求”链路，
// 仅做连接检查，完整映射读取在后续阶段实现。
// 返回：
//   false — 暂未实现（或未连接）。
// ------------------------------------------------------------
bool ModbusTcpPort::readTag(const std::string& tag_name, double& value)
{
    if (!isConnected()) return false;

    return false;
}

// ------------------------------------------------------------
// readTags() — 读取多个标签值
// ------------------------------------------------------------
// 逐个调用 readTag 并汇总结果；只要有一个标签读取失败，
// 整体返回 false（对应位置以 0 占位）。
// 参数：
//   tags   — 要读取的标签名称列表。
//   values — 输出参数，按相同顺序返回读取到的值。
// 返回：
//   true  — 全部标签读取成功；false — 存在读取失败的标签。
// ------------------------------------------------------------
bool ModbusTcpPort::readTags(const std::vector<std::string>& tags,
    std::vector<double>& values)
{
    if (!isConnected() || tags.empty()) return false;
    values.clear();
    bool ok = true;
    for (const auto& t : tags) {
        double v = 0;
        if (readTag(t, v)) values.push_back(v);
        else { values.push_back(0); ok = false; }
    }
    return ok;
}

// ------------------------------------------------------------
// writeTag() — 写入单个标签值
// ------------------------------------------------------------
// 目前尚未实现完整的“位号 → Modbus 地址 → 写请求”链路，
// 仅做连接检查与调试日志。
// 返回：
//   false — 暂未实现（或未连接）。
// ------------------------------------------------------------
bool ModbusTcpPort::writeTag(const std::string& tag_name, double value)
{
    if (!isConnected()) return false;
    LOG_DEBUG("ModbusTCP: writeTag({}={}) - not implemented", tag_name, value);
    return false;
}

// ------------------------------------------------------------
// protocolName() — 获取 Modbus TCP 协议名称
// ------------------------------------------------------------
std::string ModbusTcpPort::protocolName() const
{
    return "Modbus TCP";
}

// ------------------------------------------------------------
// poll() — 执行一次轮询（批量读写所有配置的标签）
// ------------------------------------------------------------
// 完整轮询实现（批量读、异步响应、超时重试等）在后续阶段提供，
// 当前返回 false 表示暂不可用。
// ------------------------------------------------------------
bool ModbusTcpPort::poll()
{
    return false;
}

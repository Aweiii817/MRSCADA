// ============================================================
// ModbusRtuPort.cpp — Modbus RTU 端口实现
// ============================================================
//
// 实现 ModbusRtuPort 的连接、断开、状态查询与读写接口。
// 底层使用 Qt 的 QModbusRtuSerialMaster 完成串口 Modbus RTU
// 帧的组帧、CRC 校验与收发。
// ============================================================

#include "ModbusRtuPort.h"   // Modbus RTU 端口类声明
#include "Logger.h"          // 日志工具
#include <QVariant>          // QVariant（连接参数值的统一类型）
#include <QSerialPort>       // 串口类型定义（QModbusDevice 参数）
#include <QModbusDataUnit>   // Modbus 数据单元（预留用于读写组帧）

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 此处不创建底层对象；实际的 QModbusRtuSerialMaster 在 connect()
// 中按需创建，保证每次连接都使用最新配置。
// ------------------------------------------------------------
ModbusRtuPort::ModbusRtuPort()
{
}

// ------------------------------------------------------------
// 析构函数
// ------------------------------------------------------------
// 释放前先断开连接，确保串口资源被正确归还给系统。
// ------------------------------------------------------------
ModbusRtuPort::~ModbusRtuPort()
{
    disconnect();
}

// ------------------------------------------------------------
// configureSerialPort() — 配置串口参数
// ------------------------------------------------------------
// 将配置结构体中的各项参数写入底层 QModbus 主站对象，
// 这是建立物理连接前的必要步骤。
// 参数：
//   cfg — Modbus RTU 配置（串口号、波特率、校验位等）。
// ------------------------------------------------------------
void ModbusRtuPort::configureSerialPort(const ModbusRtuConfig& cfg)
{
    // 设置串口名称（如 "COM1"）
    m_modbus->setConnectionParameter(
        QModbusDevice::SerialPortNameParameter,
        QString::fromStdString(cfg.port_name));
    // 设置波特率
    m_modbus->setConnectionParameter(
        QModbusDevice::SerialBaudRateParameter,
        cfg.baudrate);
    // 设置数据位
    m_modbus->setConnectionParameter(
        QModbusDevice::SerialDataBitsParameter,
        cfg.data_bits);

    // 设置校验位：Qt 用整数表示校验方式
    //   0 = 无校验，1 = 偶校验，2 = 奇校验
    int parity = 0;
    switch (cfg.parity) {
    case 'E': case 'e': parity = 1; break;   // 偶校验
    case 'O': case 'o': parity = 2; break;   // 奇校验
    default: parity = 0; break;              // 默认无校验
    }
    m_modbus->setConnectionParameter(
        QModbusDevice::SerialParityParameter, parity);

    // 设置停止位
    m_modbus->setConnectionParameter(
        QModbusDevice::SerialStopBitsParameter,
        cfg.stop_bits);

    // 设置超时时间（毫秒）
    m_modbus->setTimeout(cfg.timeout_ms);
    // 设置重试次数（QModbusDevice 的 setNumberOfRetries 语义为
    // “额外重试次数”，故传入 max_retries - 1 表示总共尝试 max_retries 次）
    m_modbus->setNumberOfRetries(cfg.max_retries - 1);
}

// ------------------------------------------------------------
// connect() — 连接 Modbus RTU 设备
// ------------------------------------------------------------
// 流程：
//   1. 若已连接则先断开，避免重复连接；
//   2. 将传入的通用配置转换为 RTU 配置（dynamic_cast 判断）；
//   3. 创建 QModbusRtuSerialMaster 主站对象并配置串口参数；
//   4. 尝试连接，据结果更新状态并记录日志。
// 参数：
//   config — 设备通信配置（实际应为 ModbusRtuConfig 的派生对象）。
// 返回：
//   true  — 连接成功；false — 连接失败。
// ------------------------------------------------------------
bool ModbusRtuPort::connect(const DeviceConfig& config)
{
    // 如果已经连接，则先断开
    if (m_modbus && m_modbus->state() == QModbusDevice::ConnectedState) {
        disconnect();
    }

    // 将通用配置转换为 RTU 配置（动态类型判断）
    const auto* rtu_config = dynamic_cast<const ModbusRtuConfig*>(&config);
    if (rtu_config) {
        m_config = *rtu_config;
    }
    else {
        // 传入的并非 RTU 配置时，沿用成员 m_config 的默认值
        LOG_WARN("ModbusRtuPort: using default config");
    }

    // 创建 Modbus RTU 主站对象
    m_modbus = std::make_unique<QModbusRtuSerialMaster>();

    // 配置串口参数
    configureSerialPort(m_config);

    // 尝试连接设备
    if (m_modbus->connectDevice()) {
        m_status = DeviceStatus::CONNECTED;
        LOG_INFO("ModbusRTU: connected to {} ({} baud, {}N1)",
            m_config.port_name, m_config.baudrate);
        return true;
    }
    else {
        m_status = DeviceStatus::FAULT;
        LOG_ERROR("ModbusRTU: failed to connect to {} - {}",
            m_config.port_name,
            m_modbus->errorString().toStdString());
        return false;
    }
}

// ------------------------------------------------------------
// disconnect() — 断开 Modbus RTU 设备连接
// ------------------------------------------------------------
// 断开底层连接并释放主站对象，状态置为“未连接”。
// ------------------------------------------------------------
void ModbusRtuPort::disconnect()
{
    if (m_modbus) {
        m_modbus->disconnectDevice();   // 断开底层设备
        m_modbus.reset();               // 释放主站对象
    }
    m_status = DeviceStatus::DISCONNECTED;
    LOG_INFO("ModbusRTU: disconnected");
}

// ------------------------------------------------------------
// isConnected() — 检查 Modbus RTU 设备是否已连接
// ------------------------------------------------------------
// 返回：
//   true  — 主站对象存在且处于已连接状态；
//   false — 未创建主站或未连接。
// ------------------------------------------------------------
bool ModbusRtuPort::isConnected() const
{
    return m_modbus &&
        m_modbus->state() == QModbusDevice::ConnectedState;
}

// ------------------------------------------------------------
// status() — 获取 Modbus RTU 设备连接状态
// ------------------------------------------------------------
DeviceStatus ModbusRtuPort::status() const
{
    return m_status;
}

// ------------------------------------------------------------
// readTag() — 读取单个标签值
// ------------------------------------------------------------
// 目前尚未实现完整的“位号 → Modbus 地址 → 读请求”链路，
// 仅做连接检查与调试日志，完整映射读取在后续阶段实现。
// 返回：
//   false — 暂未实现（或未连接）。
// ------------------------------------------------------------
bool ModbusRtuPort::readTag(const std::string& tag_name, double& value)
{
    if (!isConnected()) return false;
    // 通过 ModbusMapping 把位号翻译成 Modbus 地址
    // 暂未实现完整的地址映射，此处返回 false
    // 完整实现在第三阶段
    LOG_DEBUG("ModbusRTU: readTag({}) - mapping not implemented", tag_name);
    return false;
}

// ------------------------------------------------------------
// readTags() — 读取多个标签值
// ------------------------------------------------------------
// 逐个调用 readTag 并汇总结果；只要有一个标签读取失败，
// 整体返回 false（对应位置以 0.0 占位）。
// 参数：
//   tags   — 要读取的标签名称列表。
//   values — 输出参数，按相同顺序返回读取到的值。
// 返回：
//   true  — 全部标签读取成功；false — 存在读取失败的标签。
// ------------------------------------------------------------
bool ModbusRtuPort::readTags(const std::vector<std::string>& tags,
    std::vector<double>& values)
{
    if (!isConnected() || tags.empty()) return false;

    values.clear();
    bool all_ok = true;

    for (const auto& tag : tags) {
        double val = 0.0;
        if (readTag(tag, val)) {
            values.push_back(val);
        }
        else {
            values.push_back(0.0);   // 读取失败用 0.0 占位
            all_ok = false;
        }
    }

    return all_ok;
}

// ------------------------------------------------------------
// writeTag() — 写入单个标签值
// ------------------------------------------------------------
// 目前尚未实现完整的“位号 → Modbus 地址 → 写请求”链路，
// 仅做连接检查与调试日志。
// 返回：
//   false — 暂未实现（或未连接）。
// ------------------------------------------------------------
bool ModbusRtuPort::writeTag(const std::string& tag_name, double value)
{
    if (!isConnected()) return false;

    LOG_DEBUG("ModbusRTU: writeTag({}={}) - not implemented", tag_name, value);
    return false;
}

// ------------------------------------------------------------
// protocolName() — 获取 Modbus RTU 协议名称
// ------------------------------------------------------------
std::string ModbusRtuPort::protocolName() const
{
    return "Modbus RTU";
}

// ------------------------------------------------------------
// poll() — 执行一次轮询（批量读写所有配置的标签）
// ------------------------------------------------------------
// 完整轮询需考虑批量读取优化（连续地址合并为一次请求）、
// 异步响应处理（QModbusReply）、超时重试、从站无响应错误处理等，
// 复杂度较高，当前阶段返回 false 表示暂不可用。
// ------------------------------------------------------------
bool ModbusRtuPort::poll()
{
    // Modbus RTU 的轮询实现：
    // 1. 遍历所有配置的标签
    // 2. 逐个发送读取请求
    // 3. 将结果写入 RealTimeDB
    //
    // ⚠️ 完整的轮询实现比较复杂，需要考虑：
    //   - 批量读取优化（连续地址合并成一次请求）
    //   - 异步响应处理（QModbusReply）
    //   - 超时重试
    //   - 从站无响应时的错误处理
    //
    // 第三阶段提供完整实现，目前返回 false 表示暂不可用
    return false;
}

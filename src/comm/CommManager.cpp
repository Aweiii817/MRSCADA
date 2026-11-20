// ============================================================
// CommManager.cpp — 通信管理器实现
// ============================================================
//
// 实现 CommManager 的单例获取、初始化、设备增删查、以及统一的
// 启动 / 停止所有设备等逻辑。通信端口实例通过 std::unique_ptr
// 独占管理，随设备条目的移除自动释放。
// ============================================================

#include "CommManager.h"           // 通信管理器类声明
#include "Logger.h"                // 日志工具（spdlog 封装）
#include "ConfigManager.h"         // 配置管理器（读取设备列表配置）
#include "modbus/ModbusRtuPort.h"  // Modbus RTU 端口（串口通信）
#include "modbus/ModbusTcpPort.h"  // Modbus TCP 端口（以太网通信）
#include <QDateTime>               // 时间戳（预留用于轮询计时）
#include <algorithm>               // std::find_if（按条件查找设备）

// ------------------------------------------------------------
// instance() — 获取通信管理器单例实例
// ------------------------------------------------------------
// 使用函数内静态变量实现线程安全的懒汉单例：
//   首次调用时构造，之后每次调用返回同一实例。
// ------------------------------------------------------------
CommManager& CommManager::instance()
{
    static CommManager inst;
    return inst;
}

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 将自身作为无父对象的 QObject 创建（单例由静态变量持有，
// 生命周期与进程一致，无需挂靠父对象树）。
// ------------------------------------------------------------
CommManager::CommManager()
    : QObject(nullptr)
{
}

// ------------------------------------------------------------
// initialize() — 初始化通信管理器
// ------------------------------------------------------------
// 从 ConfigManager 读取通信设备列表，按每台设备的 protocol 字段
// 实例化对应的通信端口对象，并注册到管理器中统一管理。
// 说明：
//   - 目前支持 modbus_rtu / modbus_tcp 两种协议；
//   - simulation 协议不在此处实例化（SimPort 依赖仿真模块的
//     SimEngine，而 comm 模块不反向依赖 simulation），仍由 app
//     入口在 main.cpp 中创建后通过 addDevice() 注入。
// ------------------------------------------------------------
void CommManager::initialize()
{
    // 读取配置中的设备数量（无配置时默认为 0）
    auto& cfg = ConfigManager::instance();
    int device_count = cfg.getJsonValue<int>("comm.device_count", 0);

    // 遍历配置中的每台设备，逐一实例化对应端口
    for (int i = 0; i < device_count; ++i) {
        // 按 comm.devices[i] 的路径拼接各配置键
        std::string prefix = "comm.devices[" + std::to_string(i) + "]";
        std::string name = cfg.getJsonValue<std::string>(prefix + ".name", "");
        std::string protocol = cfg.getJsonValue<std::string>(prefix + ".protocol", "");

        // 名称或协议为空时跳过该条配置
        if (name.empty() || protocol.empty()) {
            LOG_WARN("CommManager: device[{}] missing name or protocol, skipped", i);
            continue;
        }

        // 根据协议类型实例化不同端口
        if (protocol == "modbus_rtu") {
            // 构造 Modbus RTU 配置
            ModbusRtuConfig config;
            config.device_name = name;
            config.protocol_type = protocol;
            config.poll_interval_ms = cfg.getJsonValue<int>(prefix + ".poll_interval_ms", 500);
            config.timeout_ms = cfg.getJsonValue<int>(prefix + ".timeout_ms", 1000);
            config.max_retries = cfg.getJsonValue<int>(prefix + ".max_retries", 3);

            // 串口专有参数
            config.port_name = cfg.getJsonValue<std::string>(prefix + ".port_name", "COM1");
            config.baudrate = cfg.getJsonValue<int>(prefix + ".baudrate", 19200);
            config.data_bits = cfg.getJsonValue<int>(prefix + ".data_bits", 8);
            config.stop_bits = cfg.getJsonValue<int>(prefix + ".stop_bits", 1);
            // 校验位在 JSON 中以字符串存储（如 "N"），取首字符
            std::string parity = cfg.getJsonValue<std::string>(prefix + ".parity", "N");
            config.parity = parity.empty() ? 'N' : parity[0];

            addDevice(std::make_unique<ModbusRtuPort>(), config);
        }
        else if (protocol == "modbus_tcp") {
            // 构造 Modbus TCP 配置
            ModbusTcpConfig config;
            config.device_name = name;
            config.protocol_type = protocol;
            config.poll_interval_ms = cfg.getJsonValue<int>(prefix + ".poll_interval_ms", 500);
            config.timeout_ms = cfg.getJsonValue<int>(prefix + ".timeout_ms", 1000);
            config.max_retries = cfg.getJsonValue<int>(prefix + ".max_retries", 3);

            // 以太网专有参数
            config.host = cfg.getJsonValue<std::string>(prefix + ".host", "127.0.0.1");
            config.port = cfg.getJsonValue<int>(prefix + ".port", 502);

            addDevice(std::make_unique<ModbusTcpPort>(), config);
        }
        else {
            // 未支持的协议类型：记录警告并跳过
            LOG_WARN("CommManager: device '{}' has unsupported protocol '{}', skipped",
                name, protocol);
        }
    }

    m_initialized = true;
    LOG_INFO("CommManager initialized with {} configured devices", device_count);
}

// ------------------------------------------------------------
// startAll() — 启动所有设备的通信轮询
// ------------------------------------------------------------
// 遍历设备列表，对存在端口实例且尚未连接的设备执行 connect()。
// 连接结果通过日志记录，供排查问题使用。
// ------------------------------------------------------------
void CommManager::startAll()
{
    for (auto& entry : m_devices) {
        if (entry.port && !entry.port->isConnected()) {
            bool ok = entry.port->connect(entry.config);
            if (ok) {
                LOG_INFO("CommManager: device '{}' connected", entry.name);
            }
            else {
                LOG_WARN("CommManager: device '{}' connect failed", entry.name);
            }
        }
    }
    LOG_INFO("CommManager: started all devices");
}

// ------------------------------------------------------------
// stopAll() — 停止所有设备的通信轮询
// ------------------------------------------------------------
// 遍历设备列表，对存在端口实例且已连接的设备执行 disconnect()。
// ------------------------------------------------------------
void CommManager::stopAll()
{
    for (auto& entry : m_devices) {
        if (entry.port && entry.port->isConnected()) {
            entry.port->disconnect();
        }
    }
    LOG_INFO("CommManager: stopped all devices");
}

// ------------------------------------------------------------
// addDevice() — 添加设备
// ------------------------------------------------------------
// 先检查是否已存在同名设备（避免重复添加），再构造设备条目并
// 接管端口实例的所有权，加入设备列表。
// 参数：
//   port   — 通信端口实例（独占所有权，被管理器接管）。
//   config — 设备通信配置。
// 返回：
//   成功返回设备名称；同名设备已存在则返回空字符串。
// ------------------------------------------------------------
std::string CommManager::addDevice(
    std::unique_ptr<ICommunicationPort> port,
    const DeviceConfig& config)
{
    // 检查是否已存在同名设备
    for (const auto& entry : m_devices) {
        if (entry.name == config.device_name) {
            LOG_WARN("CommManager: device '{}' already exists", config.device_name);
            return "";
        }
    }

    // 构建设备条目，转移端口实例所有权
    DeviceEntry entry;
    entry.name = config.device_name;
    entry.port = std::move(port);
    entry.config = config;
    entry.fail_count = 0;

    m_devices.push_back(std::move(entry));
    LOG_INFO("CommManager: added device '{}' ({})",
        config.device_name, config.protocol_type);

    return config.device_name;
}

// ------------------------------------------------------------
// removeDevice() — 移除设备
// ------------------------------------------------------------
// 按名称查找设备，若已连接则先断开，再从列表中移除。
// 参数：
//   device_name — 要移除的设备名称。
// 返回：
//   true  — 移除成功；
//   false — 未找到指定设备。
// ------------------------------------------------------------
bool CommManager::removeDevice(const std::string& device_name)
{
    // 按名称查找目标设备
    auto it = std::find_if(m_devices.begin(), m_devices.end(),
        [&](const DeviceEntry& e) { return e.name == device_name; });

    if (it != m_devices.end()) {
        // 若设备仍处于连接状态，先主动断开
        if (it->port && it->port->isConnected()) {
            it->port->disconnect();
        }
        m_devices.erase(it);
        LOG_INFO("CommManager: removed device '{}'", device_name);
        return true;
    }
    return false;
}

// ------------------------------------------------------------
// getPort() — 获取设备的通信端口接口
// ------------------------------------------------------------
// 按名称遍历查找，返回对应端口的裸指针（不转移所有权）。
// 参数：
//   device_name — 要查找的设备名称。
// 返回：
//   找到返回端口指针；未找到返回 nullptr。
// ------------------------------------------------------------
ICommunicationPort* CommManager::getPort(const std::string& device_name)
{
    for (auto& entry : m_devices) {
        if (entry.name == device_name) {
            return entry.port.get();
        }
    }
    return nullptr;
}

// ------------------------------------------------------------
// getAllStatus() — 获取所有设备状态
// ------------------------------------------------------------
// 遍历设备列表，收集每个设备名称与其当前连接状态。
// 若端口实例不存在，则默认视为“未连接”。
// 返回：
//   （设备名称，连接状态）配对列表。
// ------------------------------------------------------------
std::vector<std::pair<std::string, DeviceStatus>> CommManager::getAllStatus() const
{
    std::vector<std::pair<std::string, DeviceStatus>> result;
    for (const auto& entry : m_devices) {
        DeviceStatus status = DeviceStatus::DISCONNECTED;
        if (entry.port) {
            status = entry.port->status();
        }
        result.emplace_back(entry.name, status);
    }
    return result;
}

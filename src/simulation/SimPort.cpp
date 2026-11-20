// ============================================================
// SimPort.cpp — 仿真通信端口实现
// ============================================================
//
// 实现 ICommunicationPort 接口，把仿真引擎包装成一个标准通信设备。
// 写入操作映射为模型阀门开度；读取操作直接返回 false，因为仿真
// 数据由引擎直接写入实时数据库，上层统一从实时库读取。
// ============================================================
#include "SimPort.h"
#include "Logger.h"

// 构造函数：保存外部传入的仿真引擎指针
SimPort::SimPort(SimEngine* engine)
    : m_engine(engine)
{
}

// 连接设备：启动仿真引擎（轮询间隔取自设备配置），并标记为已连接
bool SimPort::connect(const DeviceConfig& config)
{
    // 引擎指针为空则连接失败
    if (!m_engine) return false;

	// 以配置的轮询间隔启动仿真引擎
    m_engine->start(config.poll_interval_ms);
    m_connected = true;
    m_status = DeviceStatus::CONNECTED;

    LOG_INFO("SimPort: started simulation (interval={}ms)",
        config.poll_interval_ms);
    return true;
}

// 断开设备：停止仿真引擎
void SimPort::disconnect()
{
	// 停止仿真引擎（若引擎存在）
    if (m_engine) {
        m_engine->stop();
    }
    m_connected = false;
    m_status = DeviceStatus::DISCONNECTED;
    LOG_INFO("SimPort: stopped");
}

// 检查仿真通信端口是否已连接（连接标志且引擎在运行）
bool SimPort::isConnected() const
{
    return m_connected && m_engine && m_engine->isRunning();
}

// 获取仿真通信端口的设备连接状态
DeviceStatus SimPort::status() const
{
    return m_status;
}

// 读取单个标签值：仿真端口不支持读取，直接返回 false
bool SimPort::readTag(const std::string& tag_name, double& value)
{
    return false;
}

// 读取多个标签值：仿真端口不支持读取，直接返回 false
bool SimPort::readTags(const std::vector<std::string>& tags,
    std::vector<double>& values)
{
    return false;
}

// 写入单个标签值：将值写入仿真模型的对应阀门开度
bool SimPort::writeTag(const std::string& tag_name, double value)
{
    if (m_engine) {
        m_engine->model().setValve(tag_name, value);   // 映射为阀门开度
        LOG_DEBUG("SimPort: wrote {}={}", tag_name, value);
        return true;
    }
    return false;
}

// 获取仿真通信端口的协议名称
std::string SimPort::protocolName() const
{
    return "Simulation Engine";
}

// 执行一次轮询：仿真端口无需轮询，直接返回仿真引擎的运行状态
bool SimPort::poll()
{
    return m_engine && m_engine->isRunning();
}

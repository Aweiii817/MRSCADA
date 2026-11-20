// ============================================================
// SimPort.h — 仿真通信端口
// ============================================================
//
// SimPort 实现 ICommunicationPort 接口，把"仿真引擎"包装成一个
// 标准通信设备，从而接入 CommManager 统一管理。这样上层（如
// 通信管理器、配方执行器）无需关心数据来源是真实 PLC 还是仿真，
// 都可以通过同一套接口读写。
//
// 其核心行为：
//   connect()    → 启动仿真引擎，置为已连接
//   disconnect() → 停止仿真引擎，置为断开
//   writeTag()   → 将写入值映射为仿真模型的阀门开度
//   readTag()    → 仿真端口不提供读取，直接返回 false（数据由引擎
//                  直接写入实时数据库，上层从实时库读取）
// ============================================================
#pragma once

#include "ICommunicationPort.h"   // 通信端口接口定义
#include "SimEngine.h"            // 仿真引擎

// 仿真通信端口类：将 SimEngine 封装为 ICommunicationPort 设备
class SimPort : public ICommunicationPort
{
public:
    // 构造函数：传入仿真引擎指针（端口生命周期内不拥有该引擎）
    explicit SimPort(SimEngine* engine);
    ~SimPort() override = default;

    // 连接设备：启动仿真引擎并标记为已连接
    bool connect(const DeviceConfig& config) override;
    // 断开设备：停止仿真引擎
    void disconnect() override;
    // 检查是否已连接（连接且引擎在运行）
    bool isConnected() const override;
    // 获取当前设备连接状态
    DeviceStatus status() const override;
    // 读取单个标签值（仿真端口不支持，返回 false）
    bool readTag(const std::string& tag_name, double& value) override;
    // 读取多个标签值（仿真端口不支持，返回 false）
    bool readTags(const std::vector<std::string>& tags,
        std::vector<double>& values) override;
    // 写入单个标签值（映射为仿真模型的阀门开度）
    bool writeTag(const std::string& tag_name, double value) override;
    // 获取协议名称
    std::string protocolName() const override;
    // 执行一次轮询（仿真端口无需轮询，返回引擎运行状态）
    bool poll() override;

private:
	SimEngine* m_engine;     // 仿真引擎指针（外部传入，不负责释放）
	bool m_connected = false;   // 连接状态标志
	DeviceStatus m_status = DeviceStatus::DISCONNECTED;  // 设备连接状态
};

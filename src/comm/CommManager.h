// ============================================================
// CommManager.h — 通信管理器
// ============================================================
//
// CommManager 是通信模块的“总调度器”，采用单例模式统一管理
// 所有通信设备（每个设备对应一个 ICommunicationPort 端口实例）。
// 主要职责：
//   1. 维护设备列表（名称 + 端口实例 + 配置 + 失败计数）；
//   2. 提供设备的添加 / 移除 / 查找接口；
//   3. 统一启动 / 停止所有设备的通信连接；
//   4. 对外发送设备状态变化、轮询完成等信号，供 UI 层监听。
//
// 设计说明：
//   - 继承 QObject 以获得 Qt 信号槽能力；
//   - 使用 std::unique_ptr 独占管理端口实例，避免内存泄漏；
//   - 通过单例 instance() 供全局访问，避免多实例状态不一致。
// ============================================================

#pragma once

#include <QObject>               // QObject 基类（信号槽、对象树）
#include <QTimer>                // 定时器（保留用于后续轮询调度）
#include <memory>                // std::unique_ptr 智能指针
#include <vector>                // std::vector 容器
#include "ICommunicationPort.h"  // 通信端口抽象接口与 DeviceStatus/DeviceConfig

// ------------------------------------------------------------
// CommManager — 通信管理器类
// ------------------------------------------------------------
class CommManager : public QObject
{
    Q_OBJECT   // 启用 Qt 元对象系统，使 signals 可用

public:
    // 获取通信管理器单例实例：
    //   首次调用时创建实例，后续调用返回同一个对象。
    static CommManager& instance();

    // 初始化通信管理器：
    //   从配置中读取设备列表，为后续端口实例化做准备。
    void initialize();

    // 启动所有设备的通信轮询：
    //   遍历设备列表，对未连接的设备执行 connect()。
    void startAll();

    // 停止所有设备的通信轮询：
    //   遍历设备列表，对已连接的设备执行 disconnect()。
    void stopAll();

    // 添加设备：
    //   向管理器中加入一个通信端口实例及其配置。
    // 参数：
    //   port   — 通信端口实例（独占所有权，被管理器接管）。
    //   config — 设备通信配置（含设备名称、协议类型等）。
    // 返回：
    //   成功返回设备名称；若同名设备已存在则返回空字符串。
    std::string addDevice(std::unique_ptr<ICommunicationPort> port,
        const DeviceConfig& config);

    // 移除设备：
    //   按设备名称查找并移除设备，若该设备已连接则先断开。
    // 参数：
    //   device_name — 要移除的设备名称。
    // 返回：
    //   true  — 移除成功；
    //   false — 未找到指定设备。
    bool removeDevice(const std::string& device_name);

    // 获取设备的通信端口接口：
    //   按名称查找并返回对应端口指针（不转移所有权）。
    // 参数：
    //   device_name — 要查找的设备名称。
    // 返回：
    //   找到返回端口指针；未找到返回 nullptr。
    ICommunicationPort* getPort(const std::string& device_name);

    // 获取所有设备的连接状态：
    //   返回每个设备名称与其当前 DeviceStatus 的配对列表。
    std::vector<std::pair<std::string, DeviceStatus>> getAllStatus() const;

signals:
    // 设备状态变化信号：
    //   当某设备的连接状态发生变化时发出。
    // 参数：
    //   device_name — 设备名称。
    //   status      — 新的状态（以 int 表示 DeviceStatus 枚举值）。
    void deviceStatusChanged(const QString& device_name, int status);

    // 设备轮询完成信号：
    //   当某设备完成一次轮询后发出。
    // 参数：
    //   device_name — 设备名称。
    //   success     — 本次轮询是否成功。
    void devicePollCompleted(const QString& device_name, bool success);

private:
    // 构造函数（私有）：单例模式下禁止外部直接实例化。
    CommManager();

    // 析构函数：默认实现，端口实例由 unique_ptr 自动释放。
    ~CommManager() override = default;

    // ------------------------------------------------------------
    // DeviceEntry — 设备条目结构体
    // ------------------------------------------------------------
    // 每个条目对应一个被管理的通信设备，保存其名称、端口实例、
    // 配置以及连续失败次数（用于后续重连 / 告警判断）。
    // ------------------------------------------------------------
    struct DeviceEntry {
        std::string name;                              // 设备名称
        std::unique_ptr<ICommunicationPort> port;      // 通信端口实例（独占）
        DeviceConfig config;                           // 设备通信配置
        int fail_count = 0;                            // 连续通信失败次数
    };

    std::vector<DeviceEntry> m_devices; // 设备列表（管理所有已添加的设备）
    bool m_initialized = false;         // 是否已完成初始化
};

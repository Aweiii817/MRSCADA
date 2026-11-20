// ============================================================
// ConfigManager.h — 配置管理器
//
// 负责管理两类配置：
//   1. config.json — 系统运行参数
//      通信参数、存储策略、报警阈值等
//      采用 JSON 格式，便于人工编辑
//   2. ui_state.ini — 界面状态
//      窗口位置、恢复位置、最近打开的文件等
//      使用 QSettings 管理
// ============================================================
#pragma once

#include <QString>
#include <QSettings>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

// 使用别名简化代码：Json 即 nlohmann::json
using Json = nlohmann::json;

class ConfigManager
{
public:
    // 获取配置管理器实例（单例）
    static ConfigManager& instance();

    // 初始化配置管理器
    // 传入配置目录，内部会创建目录、初始化 QSettings 并加载 config.json
    void initialize(const QString& config_dir);

    // 获取 JSON 配置值（支持点号分隔的多级 key，如 "comm.modbus_rtu.port"）
    // 若 key 不存在或类型不匹配则返回默认值 default_value
    template<typename T>
    T getJsonValue(const std::string& key, const T& default_value) const
    {
        try {
            // 从根对象开始逐级向下查找
            const Json* node = &m_json_config;
            size_t pos = 0;

            while (pos < key.size()) {
                size_t dot = key.find('.', pos);

                // 没有剩余的点号，说明是最后一级 key，直接取值
                if (dot == std::string::npos) {
                    return node->at(key.substr(pos)).get<T>();
                }

                // 取当前一级节点，继续向下一级查找
                auto segment = key.substr(pos, dot - pos);
                node = &node->at(segment);
                pos = dot + 1;
            }
        }
        catch (...) {
            // 任何异常（key 不存在、类型不匹配等）都静默处理，走默认值返回
        }
        return default_value;
    }

    // 设置 JSON 配置值（支持点号分隔的多级 key）
    // 中间节点不存在时自动创建，最后写入文件
    template<typename T>
    void setJsonValue(const std::string& key, const T& value)
    {
        Json* node = &m_json_config;
        size_t pos = 0;

        while (pos < key.size()) {
            size_t dot = key.find('.', pos);

            // 没有剩余的点号，说明是最后一级 key，直接赋值
            if (dot == std::string::npos) {
                (*node)[key.substr(pos)] = value;
                break;
            }

            // 处理中间节点，不存在则创建为 JSON 对象
            auto segment = key.substr(pos, dot - pos);
            if (!node->contains(segment)) {
                (*node)[segment] = Json::object();
            }
            node = &(*node)[segment];
            pos = dot + 1;
        }

        // 修改后立即写回文件，保证持久化
        saveJsonConfig();
    }

    // 窗口状态管理
    // 按窗口名分组，保存/加载窗口几何状态（位置、大小等）
    void saveWindowState(const QString& window_name, const QByteArray& state);
    QByteArray loadWindowState(const QString& window_name,
        const QByteArray& default_state = {});

    // 返回 QSettings 对象的引用，用于直接管理界面状态
    QSettings& settings();

private:
    ConfigManager() = default;

    // 禁止拷贝和赋值（单例模式）
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // 加载和保存 JSON 配置文件
    void loadJsonConfig();
    void saveJsonConfig();

    Json m_json_config;                          // 内存中的 JSON 配置对象
    QString m_config_dir;                        // 配置文件目录
    QString m_json_config_path;                  // config.json 文件路径
    std::unique_ptr<QSettings> m_settings;       // QSettings 对象指针，用于管理 ui_state.ini
};

// ============================================================
// ConfigManager.cpp — 配置管理器的实现
// ============================================================
// 实现 ConfigManager：
//   instance()        返回单例实例
//   initialize()      创建配置目录、初始化 QSettings、加载 JSON 配置
//   loadJsonConfig()  读取 config.json，不存在则写入默认配置
//   saveJsonConfig()  将内存配置序列化后写回文件
//   saveWindowState() / loadWindowState()  保存/加载窗口几何状态
// ============================================================
#include "ConfigManager.h"
#include "Logger.h"

#include <QDir>
#include <QFile>
#include <fstream>

// 获取配置管理器实例（单例，局部静态变量保证线程安全）
ConfigManager& ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

// 初始化配置管理器
// 步骤：记录目录 → 确保目录存在 → 初始化 QSettings → 加载 config.json
void ConfigManager::initialize(const QString& config_dir)
{
    // 保存配置文件目录路径
    m_config_dir = config_dir;

    // 检查配置文件目录是否存在，如果不存在则创建
    QDir dir(config_dir);
    if (!dir.exists()) {
        dir.mkpath(".");
        LOG_INFO("Created config directory: {}", config_dir.toStdString());
    }

    // 初始化 QSettings，用于管理界面状态（保存到 ui_state.ini，INI 格式）
    m_settings = std::make_unique<QSettings>(
        config_dir + "/ui_state.ini",
        QSettings::IniFormat
    );

    // 拼出 JSON 配置文件路径，并加载配置
    m_json_config_path = config_dir + "/config.json";
    loadJsonConfig();

    LOG_INFO("ConfigManager initialized");
}

// 从 JSON 文件加载配置，如果文件不存在则创建默认配置
void ConfigManager::loadJsonConfig()
{
    QFile file(m_json_config_path);

    // 配置文件不存在：写入一套默认配置并返回
    if (!file.exists()) {
        LOG_WARN("Config file '{}' not found, creating empty defaults",
            m_json_config_path.toStdString());

        m_json_config = Json::object();
        m_json_config["system"]["language"] = "zh_CN";
        m_json_config["system"]["theme"] = "dark";
        // 通信设备列表：CommManager::initialize() 据此按协议类型实例化端口
        m_json_config["comm"]["device_count"] = 2;
        m_json_config["comm"]["devices"] = Json::array({
            {
                {"name", "ReactorPLC"},
                {"protocol", "modbus_rtu"},
                {"port_name", "COM1"},
                {"baudrate", 19200},
                {"parity", "N"},
                {"data_bits", 8},
                {"stop_bits", 1},
                {"poll_interval_ms", 500},
                {"timeout_ms", 1000},
                {"max_retries", 3}
            },
            {
                {"name", "UtilityPLC"},
                {"protocol", "modbus_tcp"},
                {"host", "127.0.0.1"},
                {"port", 502},
                {"poll_interval_ms", 500},
                {"timeout_ms", 1000},
                {"max_retries", 3}
            }
        });
        m_json_config["storage"]["data_retention_days"] = 30;

        saveJsonConfig();
        return;
    }

    // 检查文件是否可读，如果不可读则记录错误并使用空配置
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR("Cannot open config file: {}",
            m_json_config_path.toStdString());
        m_json_config = Json::object();
        return;
    }

    // 解析 JSON 配置文件到内存中的 m_json_config 对象
    // 解析失败则记录错误并使用空配置
    try {
        QByteArray data = file.readAll();
        m_json_config = Json::parse(data.toStdString());
        LOG_INFO("Config loaded: {} ({} bytes)",
            m_json_config_path.toStdString(), data.size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config JSON: {}", e.what());
        m_json_config = Json::object();
    }
    file.close();
}

// 保存内存中的 JSON 配置到文件，保存失败则记录错误
void ConfigManager::saveJsonConfig()
{
    try {
        // dump(4)：将 JSON 对象序列化为带 4 空格缩进的字符串，便于人工阅读
        std::string json_str = m_json_config.dump(4);

        // 以文本方式打开文件并写入 JSON 字符串
        std::ofstream file(m_json_config_path.toStdString());
        if (!file.is_open()) {
            LOG_ERROR("Cannot write config to: {}",
                m_json_config_path.toStdString());
            return;
        }
        file << json_str;
        file.close();

        LOG_DEBUG("Config saved to {}", m_json_config_path.toStdString());
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to save config: {}", e.what());
    }
}

// 保存窗口状态：按窗口名分组，写入 geometry 键
void ConfigManager::saveWindowState(const QString& window_name,
    const QByteArray& state)
{
    m_settings->beginGroup(window_name);
    m_settings->setValue("geometry", state);
    m_settings->endGroup();
}

// 加载窗口状态：按窗口名读取 geometry 键，不存在则返回默认状态
QByteArray ConfigManager::loadWindowState(
    const QString& window_name,
    const QByteArray& default_state)
{
    m_settings->beginGroup(window_name);
    QByteArray state = m_settings->value("geometry", default_state).toByteArray();
    m_settings->endGroup();
    return state;
}

// 返回 QSettings 对象的引用，用于直接管理界面状态
QSettings& ConfigManager::settings()
{
    return *m_settings;
}

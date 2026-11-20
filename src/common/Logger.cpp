// ============================================================
// Logger.cpp — 日志系统的实现
// ============================================================
// 实现 Logger 类：
//   initialize()  创建双输出（控制台 + 滚动文件）的 logger 并配置级别/格式
//   instance()    返回全局唯一 logger 实例，未初始化时提供兜底实例
//   parseLevel()  将字符串日志级别转换为 spdlog 枚举
// ============================================================

#include "Logger.h"
#include <filesystem>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

// 初始化静态成员变量：进程启动时为 nullptr，首次 initialize/instance 时赋值
std::shared_ptr<spdlog::logger> Logger::s_logger = nullptr;

// 初始化日志系统
// 步骤：设置控制台编码 → 创建日志目录 → 创建控制台/文件 sink → 组装 logger → 配置级别与格式
void Logger::initialize(const std::string& log_dir,
    const std::string& log_level,
    size_t max_size, int max_files)
{
#ifdef _WIN32
    // Windows 控制台默认 GBK 编码，改为 UTF-8 以避免中文日志乱码
    SetConsoleOutputCP(CP_UTF8);
#endif

    // std::filesystem::create_directories 会检查目录是否存在，不存在则递归创建
    std::filesystem::create_directories(log_dir);

    // 创建两个 sink（输出目标）：
    //   控制台 sink：带颜色输出到标准输出
    //   文件 sink  ：滚动文件输出，单文件超 max_size 后滚动，最多保留 max_files 个
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_dir + "/ucs.log", max_size, max_files);

    // 把两个 sink 组合成一个 logger，日志会同时写入控制台和文件
    std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
    s_logger = std::make_shared<spdlog::logger>("ucs", sinks.begin(), sinks.end());

    // 设置日志级别（低于该级别的日志将被丢弃）和输出格式
    s_logger->set_level(parseLevel(log_level));
    s_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    // 设置在 error 级别时立即刷盘，避免程序崩溃时丢失错误日志
    s_logger->flush_on(spdlog::level::err);
    // 同时每 3 秒自动刷盘一次，兼顾性能与可靠性
    spdlog::flush_every(std::chrono::seconds(3));

    // 日志系统初始化完成，写一条启动日志确认配置生效
    LOG_INFO("====== UCS Logger initialized ======");
    LOG_INFO("Log directory: {}", log_dir);
    LOG_INFO("Log level: {}", log_level);
}

// 获取日志系统实例（单例）
std::shared_ptr<spdlog::logger> Logger::instance()
{
    // 若尚未初始化，则创建一个默认的控制台 logger 兜底，避免空指针调用
    if (!s_logger) {
        s_logger = spdlog::stdout_color_mt("console");
        s_logger->set_level(spdlog::level::info);
        s_logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    }
    return s_logger;
}

// 字符串映射到 spdlog 的日志级别
// 支持：trace / debug / info / warn / error / critical
spdlog::level::level_enum Logger::parseLevel(const std::string& level)
{
    // spdlog 级别枚举数值关系：trace = 0, debug = 1, info = 2, warn = 3, err = 4, critical = 5
    if (level == "trace")    return spdlog::level::trace;
    if (level == "debug")    return spdlog::level::debug;
    if (level == "info")     return spdlog::level::info;
    if (level == "warn")     return spdlog::level::warn;
    if (level == "error")    return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    // 无法识别的字符串默认按 info 级别处理
    return spdlog::level::info;
}

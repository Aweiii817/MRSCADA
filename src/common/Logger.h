// ============================================================
// Logger.h — 日志系统封装
// ============================================================
// 基于 spdlog 的日志封装，提供：
//   1. 统一的日志初始化（控制台 + 滚动文件双输出）
//   2. 全局唯一的日志实例获取接口
//   3. 便捷的日志宏（LOG_TRACE / LOG_DEBUG / LOG_INFO / LOG_WARN / LOG_ERROR）
// ============================================================

#pragma once

// 引入 spdlog 相关头文件
//   spdlog.h                   日志核心接口
//   stdout_color_sinks.h       带颜色的控制台输出
//   rotating_file_sink.h       按大小滚动的文件输出
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

// 标准库：智能指针与字符串
#include <memory>
#include <string>

class Logger
{
public:
    // 初始化日志系统
    //   log_dir    日志文件保存目录（默认 "logs"）
    //   log_level  日志级别字符串（默认 "info"）
    //   max_size   单个日志文件最大字节数，超过后自动滚动（默认 10MB）
    //   max_files  保留的滚动日志文件最大数量（默认 7 个）
    static void initialize(
        const std::string& log_dir = "logs",
        const std::string& log_level = "info",
        size_t max_size = 10 * 1024 * 1024,
        int max_files = 7);

    // 获取日志系统实例（单例）
    // 若尚未初始化，则创建一个默认的控制台 logger 兜底，避免空指针调用
    static std::shared_ptr<spdlog::logger> instance();

private:
    // 静态成员变量：保存日志系统实例，整个进程共享一份
    static std::shared_ptr<spdlog::logger> s_logger;

    // 将字符串形式的日志级别映射为 spdlog 的枚举值
    // 无法识别的字符串默认返回 info 级别
    static spdlog::level::level_enum parseLevel(const std::string& level);
};

// 便捷日志宏：统一通过 Logger::instance() 取得实例后调用对应级别日志
// 使用 SPDLOG_LOGGER_CALL 以支持可变参数（{} 占位符 + 参数）
#define LOG_TRACE(...)    SPDLOG_LOGGER_CALL(Logger::instance().get(), spdlog::level::trace, __VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_LOGGER_CALL(Logger::instance().get(), spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_CALL(Logger::instance().get(), spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_CALL(Logger::instance().get(), spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_CALL(Logger::instance().get(), spdlog::level::err, __VA_ARGS__)

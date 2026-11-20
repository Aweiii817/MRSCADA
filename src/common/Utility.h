// ============================================================
// Utility.h — 通用小工具函数
// ============================================================
// 提供一组常用的内联工具函数：
//   timestampString()  格式化当前时间字符串
//   generateId()       生成 8 位十六进制随机 ID
//   toQString()        字符串类型安全转换
//   toStdString()      字符串类型安全转换
// 全部为 inline 函数，可被多个编译单元包含而不会产生重复定义。
// ============================================================
#pragma once

#include <QString>       // Qt 字符串
#include <QDateTime>     // Qt 日期时间
#include <string>        // 标准库字符串
#include <random>        // 随机数引擎与分布

// namespace（命名空间）：将一组相关的函数组织到一起，能有效避免命名冲突
namespace Utility {

    // 获取格式化后的当前时间字符串
    // format 为 Qt 时间格式串，默认 "yyyy-MM-dd_HH-mm-ss"（如 2026-09-10_23-26-15）
    inline QString timestampString(const QString& format = "yyyy-MM-dd_HH-mm-ss")
    {
        return QDateTime::currentDateTime().toString(format);
    }

    // 生成 8 位十六进制随机 ID（字符范围 0-9、a-f）
    // rng 与 dist 为 static，进程内复用同一个随机引擎，避免每次调用重新初始化
    inline std::string generateId()
    {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_int_distribution<int> dist(0, 15);
        static const char* hex = "0123456789abcdef";

        // 预置 8 个 '0'，随后逐个替换为随机十六进制字符
        std::string id(8, '0');
        for (auto& c : id) {
            c = hex[dist(rng)];
        }
        return id;
    }

    // std::string 到 QString 的安全转换
    inline QString toQString(const std::string& str)
    {
        return QString::fromStdString(str);
    }

    // QString 到 std::string 的安全转换
    inline std::string toStdString(const QString& qstr)
    {
        return qstr.toStdString();
    }

}

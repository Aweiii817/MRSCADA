// ============================================================
// HistoryDB.h — 历史数据库
// ============================================================
//
// HistoryDB 是历史数据存储模块（单例），基于 SQLite 数据库实现。
// 它周期性地（快照定时器）将实时数据库（RealTimeDB）中的位号数据
// 写入按“月份”分表的 SQLite 表中，并提供按位号与时间范围查询、
// 以及导出 CSV 的能力。
//
// 主要功能：
//   - initialize()：初始化数据库文件与连接；
//   - startSnapshot() / stopSnapshot()：启停定时快照存储；
//   - queryHistory()：查询指定位号在时间区间内的历史记录；
//   - exportToCsv()：将历史记录导出为 CSV 文件。
//
// 存储优化：
//   - 按月分表（data_yyyyMM），便于按时间归档与清理；
//   - 死区（deadband）过滤：仅当数值变化超过一定百分比才记录；
//   - 内存缓冲批量写入：达到阈值后一次性事务提交，减少磁盘 I/O。
// ============================================================

#pragma once

#include <QObject>          // QObject 基类（用于信号槽/定时器）
#include <QSqlDatabase>     // SQLite 数据库连接
#include <QTimer>           // 快照定时器
#include <vector>           // std::vector 缓冲/结果容器
#include <string>           // std::string
#include "tag/TagDataPoint.h"  // 位号数据点定义

// 历史记录结构体：表示一条查询结果（由 queryHistory 返回）
struct HistoryRecord
{
    std::string tag_name;   // 位号名（如 "TE-101"）
    double value;           // 数值（缩放后的工程值）
    int quality;            // 数据质量码
    qint64 timestamp_ms;    // 时间戳（毫秒，Unix 纪元）
};

class HistoryDB : public QObject
{
    Q_OBJECT

public:
    // 单例访问接口（Meyers 单例，线程安全）
    static HistoryDB& instance();

    // 初始化数据库：创建目录、打开连接、设置 PRAGMA
    // 参数 db_path 为 SQLite 文件路径
    bool initialize(const QString& db_path);

    // 启动定时快照存储，interval_ms 为采样间隔（默认 5 秒）
    void startSnapshot(int interval_ms = 5000);

    // 停止快照存储，并将缓冲区剩余记录落盘
    void stopSnapshot();

    // 查询指定位号在 [start_ms, end_ms] 区间内的历史记录，
    // 最多返回 max_rows 条（默认 2000），按时间升序排列
    std::vector<HistoryRecord> queryHistory(
        const std::string& tag_name,
        qint64 start_ms,
        qint64 end_ms,
        int max_rows = 2000);

    // 将历史记录导出为 CSV 文件，返回是否成功
    bool exportToCsv(const std::vector<HistoryRecord>& records,
        const QString& file_path);

private slots:
    // 快照定时器超时槽函数：采集并缓冲当前实时数据
    void onSnapshotTimer();

private:
    // 构造函数与析构函数私有化，仅通过 instance() 获取实例
    HistoryDB();
    ~HistoryDB() override;

    // 确保指定时间戳对应的月度数据表存在，并返回表名
    QString ensureMonthTable(qint64 timestamp_ms);

    // 将缓冲区中的记录批量写入数据库（事务提交）
    void flushBuffer();

    // 判断某位号的当前值是否需要存储（死区过滤）
    bool shouldStore(const std::string& tag_name, double value);

    // 待写入数据库的缓冲记录结构体
    struct PendingRecord {
        std::string tag_name;   // 位号名
        double value;           // 数值
        int quality;            // 质量码
        qint64 timestamp_ms;    // 时间戳（毫秒）
    };

    QSqlDatabase m_db;                          // SQLite 数据库连接
    QTimer* m_snapshot_timer = nullptr;         // 快照定时器
    std::vector<PendingRecord> m_buffer;        // 待写入记录缓冲区
    static constexpr int FLUSH_THRESHOLD = 50;  // 缓冲区达到 50 条即落盘

    // 每个位号最近一次已存储的数值（用于死区判断）
    std::unordered_map<std::string, double> m_last_stored;
    double m_deadband_percent = 2.0;            // 死区阈值（变化超过 2% 才存储）

    bool m_initialized = false;                 // 是否已初始化
};

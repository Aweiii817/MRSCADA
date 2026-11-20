// ============================================================
// HistoryDB.cpp — 历史数据库实现
// ============================================================
//
// 实现 HistoryDB 历史数据库单例：负责 SQLite 建库、按月分表、
// 定时快照采集、死区过滤、批量落盘、历史查询与 CSV 导出。
// ============================================================

#include "HistoryDB.h"
#include "rtdb/RealTimeDB.h"   // 实时数据库（快照数据来源）
#include "Logger.h"            // 日志工具

#include <QSqlQuery>           // SQL 查询执行
#include <QSqlError>           // SQL 错误信息
#include <QFile>               // 文件读写（CSV 导出）
#include <QTextStream>         // 文本流（CSV 写入）
#include <QDateTime>           // 时间戳转换
#include <QDir>                // 目录操作
#include <QFileInfo>           // 文件路径信息

// ------------------------------------------------------------
// instance() — 单例访问
// ------------------------------------------------------------
// Meyers 单例：函数内静态局部变量在首次调用时构造，
// 由 C++11 保证线程安全，无需手动加锁。
// ------------------------------------------------------------
HistoryDB& HistoryDB::instance()
{
    static HistoryDB inst;
    return inst;
}

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 仅初始化基类 QObject，数据库连接在 initialize() 中建立。
// ------------------------------------------------------------
HistoryDB::HistoryDB()
    : QObject(nullptr)
{
}

// ------------------------------------------------------------
// 析构函数
// ------------------------------------------------------------
// 停止快照（并将缓冲落盘），关闭数据库连接。
// ------------------------------------------------------------
HistoryDB::~HistoryDB()
{
    stopSnapshot();
    if (m_db.isOpen()) {
        m_db.close();
    }
}

// ------------------------------------------------------------
// initialize() — 初始化数据库
// ------------------------------------------------------------
// 1. 确保数据库文件所在目录存在；
// 2. 建立 QSQLITE 连接（使用具名连接，避免与其他连接冲突）；
// 3. 打开数据库；
// 4. 设置 WAL 日志模式与 NORMAL 同步级别（性能与安全的平衡）。
// 返回是否初始化成功。
// ------------------------------------------------------------
bool HistoryDB::initialize(const QString& db_path)
{
    // 确保数据库文件所在目录存在（不存在则递归创建）
    QDir dir = QFileInfo(db_path).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 建立 SQLite 数据库连接（使用独立的连接名）
    m_db = QSqlDatabase::addDatabase("QSQLITE", "history_connection");
    m_db.setDatabaseName(db_path);

    // 打开数据库，失败则记录错误并返回 false
    if (!m_db.open()) {
        LOG_ERROR("HistoryDB: failed to open database - {}",
            m_db.lastError().text().toStdString());
        return false;
    }

    // WAL：写操作先写入预写日志，提高并发读写性能
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA journal_mode=WAL");
    // NORMAL：普通同步级别，平衡性能与崩溃安全性
    pragma.exec("PRAGMA synchronous=NORMAL");

    m_initialized = true;
    LOG_INFO("HistoryDB: initialized at {}", db_path.toStdString());
    return true;
}

// ------------------------------------------------------------
// ensureMonthTable() — 确保月度数据表存在
// ------------------------------------------------------------
// 根据时间戳生成形如 data_yyyyMM 的表名（按月分表），
// 若该表不存在则创建，并建立 (tag_name, timestamp_ms) 索引，
// 最后返回表名。
// ------------------------------------------------------------
QString HistoryDB::ensureMonthTable(qint64 timestamp_ms)
{
    // 将时间戳转换为日期，取年月作为表名后缀
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp_ms);
    QString table_name = QString("data_%1")
        .arg(dt.toString("yyyyMM"));

    QSqlQuery query(m_db);

    // 建表语句：id 自增主键 + 位号 + 数值 + 质量码 + 时间戳
    QString sql = QString(
        "CREATE TABLE IF NOT EXISTS %1 ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  tag_name TEXT NOT NULL,"
        "  value REAL NOT NULL,"
        "  quality INTEGER DEFAULT 192,"
        "  timestamp_ms INTEGER NOT NULL"
        ")").arg(table_name);

    if (!query.exec(sql)) {
        LOG_ERROR("HistoryDB: create table failed - {}",
            query.lastError().text().toStdString());
    }

    // 建立 (tag_name, timestamp_ms) 复合索引，加速按位号+时间查询
    QString index_sql = QString(
        "CREATE INDEX IF NOT EXISTS idx_%1_tagtime "
        "ON %1(tag_name, timestamp_ms)").arg(table_name);
    query.exec(index_sql);

    return table_name;
}

// ------------------------------------------------------------
// startSnapshot() — 启动定时快照
// ------------------------------------------------------------
// 创建（若尚未创建）并启动快照定时器，按指定间隔触发采集。
// ------------------------------------------------------------
void HistoryDB::startSnapshot(int interval_ms)
{
    if (!m_initialized) return;

    // 懒创建定时器并连接槽函数
    if (!m_snapshot_timer) {
        m_snapshot_timer = new QTimer(this);
        connect(m_snapshot_timer, &QTimer::timeout,
            this, &HistoryDB::onSnapshotTimer);
    }
    m_snapshot_timer->start(interval_ms);
    LOG_INFO("HistoryDB: snapshot started (interval={}ms)", interval_ms);
}

// ------------------------------------------------------------
// stopSnapshot() — 停止定时快照
// ------------------------------------------------------------
// 停止定时器，并将缓冲区中尚未写入的记录立即落盘。
// ------------------------------------------------------------
void HistoryDB::stopSnapshot()
{
    if (m_snapshot_timer) {
        m_snapshot_timer->stop();
    }
    flushBuffer();
}

// ------------------------------------------------------------
// onSnapshotTimer() — 快照采集
// ------------------------------------------------------------
// 定时器超时触发：从实时数据库取出所有位号的最新值，
// 经死区过滤后加入缓冲区，缓冲区满则立即批量落盘。
// ------------------------------------------------------------
void HistoryDB::onSnapshotTimer()
{
    // 获取实时数据库中所有位号的最新值
    auto all_values = RealTimeDB::instance().getAllLatestValues();

    for (const auto& point : all_values) {
        // 死区过滤：变化过小则跳过，避免存储大量冗余数据
        if (!shouldStore(point.tag_name, point.scaled_value)) {
            continue;
        }

        // 构造缓冲记录
        PendingRecord rec;
        rec.tag_name = point.tag_name;
        rec.value = point.scaled_value;
        rec.quality = static_cast<int>(point.quality);
        rec.timestamp_ms = static_cast<qint64>(point.timestamp_ms);
        m_buffer.push_back(rec);

        // 记录该位号最近一次已存储的数值，供下次死区判断
        m_last_stored[point.tag_name] = point.scaled_value;
    }

    // 缓冲区达到阈值则立即落盘
    if (m_buffer.size() >= FLUSH_THRESHOLD) {
        flushBuffer();
    }
}

// ------------------------------------------------------------
// shouldStore() — 死区判断
// ------------------------------------------------------------
// 决定某位号当前值是否需要存储：
//   - 首次出现：总是存储；
//   - 上次值为 0：当前值非 0 才存储；
//   - 否则：变化百分比达到死区阈值（m_deadband_percent）才存储。
// 这样可抑制数值微小抖动造成的大量冗余记录。
// ------------------------------------------------------------
bool HistoryDB::shouldStore(const std::string& tag_name, double value)
{
    auto it = m_last_stored.find(tag_name);
    if (it == m_last_stored.end()) {
        return true;   // 首次出现，总是存储
    }

    double last = it->second;
    if (last == 0.0) {
        return value != 0.0;   // 从 0 变为非 0 才存储
    }

    // 计算相对变化百分比
    double change_percent = qAbs(value - last) / qAbs(last) * 100.0;
    return change_percent >= m_deadband_percent;
}

// ------------------------------------------------------------
// flushBuffer() — 批量落盘
// ------------------------------------------------------------
// 将缓冲区中的全部记录通过一个事务批量写入数据库，然后清空缓冲。
// 使用参数绑定（bindValue）而非字符串拼接，既安全又高效。
// ------------------------------------------------------------
void HistoryDB::flushBuffer()
{
    if (m_buffer.empty() || !m_initialized) return;

    // 开启事务，批量写入减少磁盘 I/O
    m_db.transaction();

    for (const auto& rec : m_buffer) {
        // 按记录时间戳确定写入哪个月度表
        QString table = ensureMonthTable(rec.timestamp_ms);

        QSqlQuery query(m_db);
        query.prepare(QString(
            "INSERT INTO %1 (tag_name, value, quality, timestamp_ms) "
            "VALUES (:tag, :val, :qua, :ts)").arg(table));

        // 参数绑定：比字符串拼接安全（防 SQL 注入）且更快
        query.bindValue(":tag", QString::fromStdString(rec.tag_name));
        query.bindValue(":val", rec.value);
        query.bindValue(":qua", rec.quality);
        query.bindValue(":ts", rec.timestamp_ms);

        if (!query.exec()) {
            LOG_ERROR("HistoryDB: insert failed - {}",
                query.lastError().text().toStdString());
        }
    }

    // 提交事务
    m_db.commit();

    LOG_DEBUG("HistoryDB: flushed {} records", m_buffer.size());
    m_buffer.clear();
}

// ------------------------------------------------------------
// queryHistory() — 查询历史记录
// ------------------------------------------------------------
// 按位号与时间区间查询历史数据。步骤：
//   1. 根据起始时间确定月度表名；
//   2. 检查该表是否存在（不存在直接返回空结果）；
//   3. 执行参数化查询，按时间升序返回最多 max_rows 条。
// ------------------------------------------------------------
std::vector<HistoryRecord> HistoryDB::queryHistory(
    const std::string& tag_name,
    qint64 start_ms,
    qint64 end_ms,
    int max_rows)
{
    std::vector<HistoryRecord> result;
    if (!m_initialized) return result;
    
    // 待优化：跨月查询
    // 由起始时间推导月度表名（假设查询区间在同一月内）
    QString table = QString("data_%1")
        .arg(QDateTime::fromMSecsSinceEpoch(start_ms).toString("yyyyMM"));

    // 检查该表是否存在（查询 sqlite_master 系统表）
    QSqlQuery check(m_db);
    check.exec(QString(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='%1'"
    ).arg(table));
    if (!check.next()) {
        return result;   // 表不存在，返回空结果
    }

    // 构造参数化查询
    QSqlQuery query(m_db);
    query.prepare(QString(
        "SELECT tag_name, value, quality, timestamp_ms "
        "FROM %1 "
        "WHERE tag_name = :tag "
        "  AND timestamp_ms >= :start "
        "  AND timestamp_ms <= :end "
        "ORDER BY timestamp_ms ASC "
        "LIMIT :limit").arg(table));

    query.bindValue(":tag", QString::fromStdString(tag_name));
    query.bindValue(":start", start_ms);
    query.bindValue(":end", end_ms);
    query.bindValue(":limit", max_rows);

    if (!query.exec()) {
        LOG_ERROR("HistoryDB: query failed - {}",
            query.lastError().text().toStdString());
        return result;
    }

    // 逐行读取结果
    while (query.next()) {
        HistoryRecord rec;
        rec.tag_name = query.value(0).toString().toStdString();
        rec.value = query.value(1).toDouble();
        rec.quality = query.value(2).toInt();
        rec.timestamp_ms = query.value(3).toLongLong();
        result.push_back(rec);
    }

    LOG_DEBUG("HistoryDB: query {} returned {} rows",
        tag_name, result.size());
    return result;
}

// ------------------------------------------------------------
// exportToCsv() — 导出 CSV
// ------------------------------------------------------------
// 将历史记录写入指定路径的 CSV 文件，包含表头与每行一条记录。
// 返回是否成功（文件能否创建并打开）。
// ------------------------------------------------------------
bool HistoryDB::exportToCsv(const std::vector<HistoryRecord>& records,
    const QString& file_path)
{
    QFile file(file_path);
    // 以只写、文本模式打开文件
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("HistoryDB: cannot open CSV file - {}",
            file_path.toStdString());
        return false;
    }

    QTextStream out(&file);

    // 写入表头
    out << "timestamp,datetime,tag_name,value,quality\n";

    // 逐条写入记录
    for (const auto& rec : records) {
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(rec.timestamp_ms);
        out << rec.timestamp_ms << ","
            << dt.toString("yyyy-MM-dd HH:mm:ss") << ","
            << QString::fromStdString(rec.tag_name) << ","
            << rec.value << ","
            << rec.quality << "\n";
    }

    file.close();
    LOG_INFO("HistoryDB: exported {} records to {}",
        records.size(), file_path.toStdString());
    return true;
}

// ============================================================
// RealTimeDB.h — 实时数据库
// ============================================================
// 进程内的实时数据库，保存所有位号的当前值与近期历史值：
//   当前值  → m_current_values（每个位号只保留最新一条）
//   历史值  → m_histories（每个位号一个 RingBuffer，容量固定）
// 对外提供单例接口，通过 Qt 信号通知界面数据变化。
// ============================================================
#pragma once

#include <QObject>
#include <unordered_map>
#include <memory>
#include "tag/TagDataPoint.h"
#include "RingBuffer.h"

class RealTimeDB : public QObject
{
    Q_OBJECT   // 使用 Qt 信号槽机制

public:
    // 获取实时数据库实例（单例）
    static RealTimeDB& instance();

    // 更新标签的原始值（来自设备/仿真，通常未做量程换算）
    // quality 默认 GOOD
    void updateTagValue(const std::string& tag_name,
        double raw_value,
        Quality quality = Quality::GOOD);

    // 设置标签的工程值（人工/业务写入，通常是已换算好的显示值）
    // quality 默认 GOOD_LOCAL_OVERRIDE
    void setTagValue(const std::string& tag_name,
        double scaled_value,
        Quality quality = Quality::GOOD_LOCAL_OVERRIDE);

    // 获取标签的当前值，成功返回 true 并写入 out_value，否则返回 false
    bool getTagValue(const std::string& tag_name,
        TagDataPoint& out_value) const;

    // 获取标签的历史数据，最多返回 count 条（默认 100）
    std::vector<TagDataPoint> getTagHistory(
        const std::string& tag_name, size_t count = 100) const;

    // 获取所有标签的最新值
    std::vector<TagDataPoint> getAllLatestValues() const;

signals:
    // 标签值变化信号（带位号名称）
    void tagValueChanged(const QString& tag_name);
    // 标签质量变化信号（带位号名称与新质量值）
    void tagQualityChanged(const QString& tag_name, int quality);

private:
    RealTimeDB();
    ~RealTimeDB() override = default;

    // 禁止拷贝和赋值（单例模式）
    RealTimeDB(const RealTimeDB&) = delete;
    RealTimeDB& operator=(const RealTimeDB&) = delete;

    // 存储每个标签的当前值
    std::unordered_map<std::string, TagDataPoint> m_current_values;
    // 存储每个标签的历史值（每个标签一个环形缓冲区）
    std::unordered_map<std::string,
        std::unique_ptr<RingBuffer<TagDataPoint>>> m_histories;

    // 保护上述两个容器并发访问的互斥锁
    mutable std::mutex m_mutex;
};

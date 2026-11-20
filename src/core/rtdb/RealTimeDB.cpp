// ============================================================
// RealTimeDB.cpp — 实时数据库的实现
// ============================================================
// 实现 RealTimeDB：
//   instance()         返回单例实例
//   updateTagValue()   更新原始值（补充标签静态信息、维护当前值与历史值）
//   setTagValue()      设置工程值
//   getTagValue()      读取当前值
//   getTagHistory()    读取历史值（最近 count 条）
//   getAllLatestValues()  读取所有标签的最新值
// ============================================================
#include "RealTimeDB.h"
#include "Logger.h"
#include "tag/TagManager.h"
#include <QDateTime>

// 获取实时数据库实例（单例，局部静态变量保证线程安全）
RealTimeDB& RealTimeDB::instance()
{
    static RealTimeDB inst;
    return inst;
}

RealTimeDB::RealTimeDB()
    : QObject(nullptr)
{
    LOG_INFO("RealTimeDB initialized");
}

// 更新标签的原始值
void RealTimeDB::updateTagValue(const std::string& tag_name,
    double raw_value, Quality quality)
{
    // 组装一个新的数据点，填充位号、原始值、质量与时间戳
    TagDataPoint point;
    point.tag_name = tag_name;
    point.raw_value = raw_value;
    point.scaled_value = raw_value;       // 此处暂将工程值直接设为原始值（无换算）
    point.quality = quality;
    point.timestamp_ms = QDateTime::currentMSecsSinceEpoch();  // 当前时间戳（毫秒）

    // 从标签管理器获取标签的静态信息，用于补充描述、单位、类型等字段
    const auto* tag_def = TagManager::instance().findTag(tag_name);
    if (tag_def) {
        point.description = tag_def->description;
        point.unit = tag_def->unit;
        point.data_type = tag_def->data_type;
        point.source_device = tag_def->source_device;
        point.source_address = tag_def->source_address;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 检查当前标签值的质量是否发生变化，若变化则发出质量变化信号
        auto value_it = m_current_values.find(tag_name);
        if (value_it != m_current_values.end()) {
            Quality old_quality = value_it->second.quality;
            if (old_quality != quality) {
                emit tagQualityChanged(
                    QString::fromStdString(tag_name),
                    static_cast<int>(quality));
            }
        }

        // 更新当前标签值
        m_current_values[tag_name] = point;

        // 更新标签历史数据，如果不存在则创建一个新的环形缓冲区（容量 600）
        auto hist_it = m_histories.find(tag_name);
        if (hist_it == m_histories.end()) {
            auto buffer = std::make_unique<RingBuffer<TagDataPoint>>(600);
            buffer->push(point);
            m_histories[tag_name] = std::move(buffer);
        }
        else {
            hist_it->second->push(point);
        }
    }

    // 值更新完成后，发出标签值变化信号（锁外发送，避免阻塞）
    emit tagValueChanged(QString::fromStdString(tag_name));
}

// 设置标签的工程值
void RealTimeDB::setTagValue(const std::string& tag_name,
    double scaled_value, Quality quality)
{
    // 组装数据点，工程值与原始值均设为传入的 scaled_value
    TagDataPoint point;
    point.tag_name = tag_name;
    point.raw_value = scaled_value;
    point.scaled_value = scaled_value;
    point.quality = quality;
    point.timestamp_ms = QDateTime::currentMSecsSinceEpoch();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 更新当前值与历史值（逻辑同 updateTagValue）
        m_current_values[tag_name] = point;
        auto hist_it = m_histories.find(tag_name);
        if (hist_it == m_histories.end()) {
            auto buffer = std::make_unique<RingBuffer<TagDataPoint>>(600);
            buffer->push(point);
            m_histories[tag_name] = std::move(buffer);
        }
        else {
            hist_it->second->push(point);
        }
    }

    emit tagValueChanged(QString::fromStdString(tag_name));
}

// 获取标签的当前值
bool RealTimeDB::getTagValue(const std::string& tag_name,
    TagDataPoint& out_value) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_current_values.find(tag_name);
    if (it != m_current_values.end()) {
        out_value = it->second;   // 拷贝一份返回
        return true;
    }
    return false;                 // 位号不存在
}

// 获取标签的历史数据，最多 count 条记录
std::vector<TagDataPoint> RealTimeDB::getTagHistory(
    const std::string& tag_name, size_t count) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_histories.find(tag_name);
    if (it != m_histories.end()) {
        auto all = it->second->readAll();
        // 如果历史数据多于需要的条数，只取最近的 count 条记录
        if (all.size() > count) {
            all.erase(all.begin(), all.end() - count);
        }
        return all;
    }
    return {};
}

// 获取所有标签的最新值
std::vector<TagDataPoint> RealTimeDB::getAllLatestValues() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<TagDataPoint> result;
    result.reserve(m_current_values.size());
    for (const auto& [name, point] : m_current_values) {
        result.push_back(point);
    }
    return result;
}

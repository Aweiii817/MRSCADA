// ============================================================
// TagManager.cpp — 标签管理器的实现
// ============================================================
// 实现 TagManager：
//   instance()        返回单例实例
//   loadFromJson()    解析 JSON 字符串并批量加载标签
//   findTag()         按位号名称快速查找
//   getTagsByDevice() 按设备名过滤标签
//   allTags() / tagCount()  获取全部标签及数量
//   addOrUpdateTag()  添加或更新单个标签
// ============================================================
#include "TagManager.h"
#include "Logger.h"
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

// 获取标签管理器实例（单例，局部静态变量保证线程安全）
TagManager& TagManager::instance()
{
    static TagManager inst;
    return inst;
}

// 从 JSON 字符串加载标签，返回成功加载的标签数量
size_t TagManager::loadFromJson(const std::string& json_str)
{
    // 清空现有标签和映射，避免残留旧数据
    m_tags.clear();
    m_tag_map.clear();

    try {
        // 解析 JSON 字符串
        Json j = Json::parse(json_str);

        // 检查 JSON 是否包含 "tags" 数组
        if (!j.contains("tags") || !j["tags"].is_array()) {
            LOG_ERROR("TagManager: JSON 格式错误：缺少 'tags' 数组");
            return 0;
        }

        // 遍历 "tags" 数组，创建 TagDataPoint 对象并保存到 m_tags 和 m_tag_map
        for (const auto& item : j["tags"]) {
            auto tag = std::make_unique<TagDataPoint>();

            // 从 JSON 对象逐字段取值（缺省时使用默认值）
            tag->tag_name = item.value("tag_name", "");
            tag->description = item.value("description", "");
            tag->unit = item.value("unit", "");
            tag->data_type = static_cast<DataType>(item.value("data_type", 5));
            tag->source_device = item.value("source_device", "");
            tag->source_address = item.value("source_address", 0);

            // 位号名称为空则跳过
            if (tag->tag_name.empty()) {
                LOG_WARN("TagManager: 跳过一条没有位号的标签");
                continue;
            }

            // 位号重复则跳过（保留先加载的）
            if (m_tag_map.find(tag->tag_name) != m_tag_map.end()) {
                LOG_WARN("TagManager: 位号 '{}' 重复，已忽略", tag->tag_name);
                continue;
            }

            // 建立名称到指针的映射，并把对象移入容器
            m_tag_map[tag->tag_name] = tag.get();
            m_tags.push_back(std::move(tag));
        }

        LOG_INFO("TagManager: 成功加载 {} 个标签", m_tags.size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("TagManager: JSON 解析失败：{}", e.what());
    }

    return m_tags.size();
}

// 查找标签，如果不存在则返回 nullptr
// 借助 unordered_map 实现 O(1) 平均时间复杂度的查找
const TagDataPoint* TagManager::findTag(const std::string& tag_name) const
{
    auto it = m_tag_map.find(tag_name);

    if (it != m_tag_map.end()) {
        return it->second;
    }

    return nullptr;
}

// 获取指定设备的所有标签
// 线性遍历所有标签，筛选出 source_device 匹配的项
std::vector<const TagDataPoint*> TagManager::getTagsByDevice(
    const std::string& device_name) const
{
    std::vector<const TagDataPoint*> result;

    for (const auto& tag : m_tags) {
        if (tag->source_device == device_name) {
            result.push_back(tag.get());
        }
    }

    return result;
}

// 获取所有标签（返回容器只读引用，避免拷贝）
const std::vector<std::unique_ptr<TagDataPoint>>& TagManager::allTags() const
{
    return m_tags;
}

// 获取标签数量
size_t TagManager::tagCount() const
{
    return m_tags.size();
}

// 添加或更新标签
void TagManager::addOrUpdateTag(std::unique_ptr<TagDataPoint> tag)
{
    // 空指针或位号为空直接忽略
    if (!tag || tag->tag_name.empty()) return;

    auto it = m_tag_map.find(tag->tag_name);
    // 如果标签已存在，则替换其内容
    if (it != m_tag_map.end()) {
        for (auto& t : m_tags) {
            if (t->tag_name == tag->tag_name) {
                // 更新映射指向新对象，并用新对象替换容器中的旧对象
                m_tag_map[tag->tag_name] = tag.get();
                t = std::move(tag);
                return;
            }
        }
    }

    // 如果标签不存在，则添加新标签
    m_tag_map[tag->tag_name] = tag.get();
    m_tags.push_back(std::move(tag));
}

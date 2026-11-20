// ============================================================
// TagManager.h — 标签管理器
//
// 功能：管理系统中"有哪些标签"的静态信息。
//
// 这里扮演的角色类似于"公司员工花名册"：
//   花名册记录每个员工的基本信息（工号）、
//   职位、部门、联系方式、所在设备等信息。
//
// 界面显示时，可以通过 TagManager 查到某个标签
// 的单元信息（位号、量程等），再通过 RealTimeDB 获取当前运行值。
// ============================================================
#pragma once

#include "TagDataPoint.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

class TagManager
{
public:
    // 获取标签管理器实例（单例）
    static TagManager& instance();

    // 从 JSON 字符串加载标签，返回成功加载的标签数量
    size_t loadFromJson(const std::string& json_str);

    // 按位号名称查找标签，不存在则返回 nullptr
    const TagDataPoint* findTag(const std::string& tag_name) const;

    // 获取指定设备的所有标签
    std::vector<const TagDataPoint*> getTagsByDevice(
        const std::string& device_name) const;

    // 获取所有标签（返回只读引用）
    const std::vector<std::unique_ptr<TagDataPoint>>& allTags() const;

    // 获取标签数量
    size_t tagCount() const;

    // 添加或更新标签（按位号名称判断，存在则替换，不存在则新增）
    void addOrUpdateTag(std::unique_ptr<TagDataPoint> tag);

private:
    TagManager() = default;

    // 禁止拷贝和赋值（单例模式）
    TagManager(const TagManager&) = delete;
    TagManager& operator=(const TagManager&) = delete;

    // 存放所有标签（独占所有权）
    std::vector<std::unique_ptr<TagDataPoint>> m_tags;
    // 位号名称到标签指针的映射，用于 O(1) 快速查找
    std::unordered_map<std::string, TagDataPoint*> m_tag_map;
};

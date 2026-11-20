// ============================================================
// ModbusMapping.cpp — 映射管理实现
// ============================================================
//
// 实现 ModbusMappingManager 的映射构建与查询逻辑。
// 核心入口 buildFromTagManager() 从实时数据库的标签管理器
// （TagManager）读取全部标签，根据标签的数据类型与源地址，
// 自动生成“位号 ↔ Modbus 寄存器地址”的映射关系。
// ============================================================

#include "ModbusMapping.h"          // 映射结构体与管理器声明
#include "tag/TagManager.h"         // 标签管理器（提供全部标签）
#include "Logger.h"                 // 日志工具

// ------------------------------------------------------------
// buildFromTagManager() — 构建映射表
// ------------------------------------------------------------
// 流程：
//   1. 清空旧的映射索引；
//   2. 从 TagManager 获取全部标签；
//   3. 遍历每个标签，依据数据类型确定寄存器类型、依据源地址
//      确定寄存器地址，生成映射并写入两份索引；
//   4. 记录构建结果日志。
// ------------------------------------------------------------
void ModbusMappingManager::buildFromTagManager()
{
    // 清空既有映射，避免重复构建时残留旧数据
    m_device_mappings.clear();
    m_tag_mappings.clear();

    // 从 TagManager 中获取所有标签（只读引用）
    const auto& all_tags = TagManager::instance().allTags();

    // 遍历所有标签，生成 Modbus 映射
    for (const auto& tag_ptr : all_tags) {
        if (!tag_ptr) continue;   // 跳过空指针（防御性判断）

        ModbusTagMapping mapping;
        mapping.tag_name = tag_ptr->tag_name;
        mapping.slave_address = 1;   // 从站地址默认固定为 1

        // 根据标签的数据类型决定 Modbus 寄存器类型：
        //   BOOL → 线圈（可读写的位）
        //   其它 → 保持寄存器（可读写的字，默认处理）
        switch (tag_ptr->data_type) {
        case DataType::BOOL:
            mapping.reg_type = ModbusRegisterType::COIL;
            break;
        default:
            mapping.reg_type = ModbusRegisterType::HOLDING_REG;
            break;
        }

        // 寄存器地址直接取自标签的源地址
        mapping.register_address = tag_ptr->source_address;
        mapping.scale = 1.0;    // 默认不缩放
        mapping.offset = 0.0;   // 默认不偏移

        // 按设备分组；若标签未指定源设备，则归入默认设备 "default"
        std::string device = tag_ptr->source_device.empty()
            ? "default" : tag_ptr->source_device;

        // 同时写入“设备维度”与“标签维度”两份索引
        m_device_mappings[device].push_back(mapping);
        m_tag_mappings[tag_ptr->tag_name] = mapping;
    }

    LOG_INFO("ModbusMapping: built {} mappings across {} devices",
        m_tag_mappings.size(), m_device_mappings.size());
}

// ------------------------------------------------------------
// findMapping() — 查找标签映射
// ------------------------------------------------------------
// 参数：
//   tag_name    — 要查找的标签名称。
//   out_mapping — 输出参数，命中时写入查找到的映射。
// 返回：
//   true  — 找到并填充 out_mapping；
//   false — 未找到该标签的映射。
// ------------------------------------------------------------
bool ModbusMappingManager::findMapping(const std::string& tag_name,
    ModbusTagMapping& out_mapping) const
{
    auto it = m_tag_mappings.find(tag_name);
    if (it != m_tag_mappings.end()) {
        out_mapping = it->second;
        return true;
    }
    return false;
}

// ------------------------------------------------------------
// getDeviceMappings() — 获取指定设备的所有映射
// ------------------------------------------------------------
// 参数：
//   device_name — 设备名称。
// 返回：
//   找到返回映射列表指针（由内部容器持有，勿释放）；
//   未找到返回 nullptr。
// ------------------------------------------------------------
const std::vector<ModbusTagMapping>* ModbusMappingManager::getDeviceMappings(
    const std::string& device_name) const
{
    auto it = m_device_mappings.find(device_name);
    if (it != m_device_mappings.end()) {
        return &it->second;
    }
    return nullptr;
}

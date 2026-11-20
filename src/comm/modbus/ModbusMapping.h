// ============================================================
// ModbusMapping.h — Modbus 地址映射
// ============================================================
//
// 本头文件定义了 Modbus 位号（标签）与 Modbus 寄存器地址之间的
// 映射关系，是“标签名 → 寄存器地址”的翻译层。
//
// 背景：上位机的业务层通过有意义的位号（如 "TI-101"、"P-101"）来
// 读写数据，而 Modbus 设备只认寄存器地址（如 保持寄存器 0x0000）。
// ModbusMapping 负责把这两者对应起来，通信端口在读写前先通过
// 映射管理器把位号翻译成具体的从站地址 + 寄存器类型 + 寄存器地址。
//
// 核心组成：
//   ModbusRegisterType     寄存器类型枚举（线圈/离散输入/保持/输入寄存器）
//   ModbusTagMapping       单个标签的映射（位号、从站、寄存器、量程换算）
//   ModbusDeviceMapping     单台设备的映射（设备名 + 标签映射列表）
//   ModbusMappingManager    映射管理器（构建、查找映射）
// ============================================================

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

// ------------------------------------------------------------
// ModbusRegisterType — Modbus 寄存器类型枚举
// ------------------------------------------------------------
// 对应 Modbus 协议中的四类数据区，决定读写方式：
//   - 线圈/保持寄存器：可读可写（常用于输出、控制）
//   - 离散输入/输入寄存器：只读（常用于传感器、状态采集）
// ------------------------------------------------------------
enum class ModbusRegisterType : uint8_t
{
    COIL,           // 线圈：可读可写，位寻址（1 位）
    DISCRETE_INPUT, // 离散输入：只读，位寻址（1 位）
    HOLDING_REG,    // 保持寄存器：可读可写，字寻址（16 位）
    INPUT_REG       // 输入寄存器：只读，字寻址（16 位）
};

// ------------------------------------------------------------
// ModbusTagMapping — Modbus 标签映射结构体
// ------------------------------------------------------------
// 描述单个标签如何映射到 Modbus 设备上的某个寄存器。
// 除寄存器地址外，还支持量程换算：
//   实际工程值 = 寄存器原始值 * scale + offset
// ------------------------------------------------------------
struct ModbusTagMapping
{
    std::string tag_name;           // 标签名称（位号，如 "TI-101"）
    int slave_address = 1;          // Modbus 从站地址（默认 1）
    ModbusRegisterType reg_type;    // 寄存器类型（线圈/离散输入/保持/输入寄存器）
    int register_address = 0;       // 寄存器地址（在该寄存器类型内的偏移）
    double scale = 1.0;             // 转换系数（原始值缩放倍数）
    double offset = 0.0;            // 转换偏移（原始值平移量）
};

// ------------------------------------------------------------
// ModbusDeviceMapping — Modbus 设备映射结构体
// ------------------------------------------------------------
// 描述单台 Modbus 设备（一个从站）所包含的全部标签映射。
// 用于按设备批量遍历其下所有标签，方便一次轮询覆盖整台设备。
// ------------------------------------------------------------
struct ModbusDeviceMapping
{
    std::string device_name;                 // 设备名称
    std::vector<ModbusTagMapping> tags;      // 该设备下的标签映射列表
};

// ------------------------------------------------------------
// ModbusMappingManager — Modbus 映射管理器
// ------------------------------------------------------------
// 负责构建并维护“位号 ↔ Modbus 地址”的映射表，供通信端口在
// 读写标签前查询。内部维护两份索引以支持不同维度的查找：
//   - m_device_mappings：按设备名聚合（用于按设备批量轮询）
//   - m_tag_mappings：按标签名索引（用于单个标签快速查找）
// ------------------------------------------------------------
class ModbusMappingManager
{
public:
    // 构建映射表：
    //   从 TagManager 读取所有标签，按标签数据类型与源地址生成
    //   对应的 Modbus 映射，并分别建立“设备维度”与“标签维度”索引。
    void buildFromTagManager();

    // 查找标签映射：
    //   按标签名查找对应的 Modbus 映射。
    // 参数：
    //   tag_name    — 要查找的标签名称。
    //   out_mapping — 输出参数，命中时写入查找到的映射。
    // 返回：
    //   true  — 找到并填充 out_mapping；
    //   false — 未找到该标签的映射。
    bool findMapping(const std::string& tag_name,
        ModbusTagMapping& out_mapping) const;

    // 获取设备所有的映射：
    //   按设备名返回该设备下的全部标签映射列表指针。
    // 参数：
    //   device_name — 设备名称。
    // 返回：
    //   找到返回映射列表指针（由内部容器持有，勿释放）；
    //   未找到返回 nullptr。
    const std::vector<ModbusTagMapping>* getDeviceMappings(
        const std::string& device_name) const;

private:
    // 设备名 -> 该设备下的标签映射列表（用于按设备批量轮询）
    std::unordered_map<std::string, std::vector<ModbusTagMapping>> m_device_mappings;
    // 标签名 -> 标签映射（用于按标签名快速查找）
    std::unordered_map<std::string, ModbusTagMapping> m_tag_mappings;
};

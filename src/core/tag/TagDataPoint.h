// ============================================================
// TagDataPoint.h — 工业数据点的基础数据结构
// ============================================================
// 定义位号（数据点）相关的核心类型：
//   DataType     数据类型枚举
//   Quality      数据质量枚举
//   TagDataPoint 单个数据点结构体（位号的静态属性 + 运行时值）
//   DataConverter 数据换算工具（量程缩放、差压流量开方）
// ============================================================
#pragma once

#include <string>
#include <cstdint>
#include <cmath>

// DataType 数据类型枚举
// 表示一个位号底层数据的存储类型，与现场设备/协议的数据类型对应
enum class DataType : uint8_t
{
    BOOL = 0,      // 布尔
    INT16 = 1,     // 16 位有符号整数
    UINT16 = 2,    // 16 位无符号整数
    INT32 = 3,     // 32 位有符号整数
    UINT32 = 4,    // 32 位无符号整数
    FLOAT = 5,     // 单精度浮点
    DOUBLE = 6,    // 双精度浮点
    STRING = 7     // 字符串
};

// Quality 数据质量枚举
// 表示数据点当前值的可信程度，数值越大质量越好（参考 OPC 质量码约定）
enum class Quality : uint8_t
{
    BAD = 0,                  // 坏值（通信中断、数据无效）
    UNCERTAIN = 64,           // 不确定（数据可疑，如超量程）
    GOOD = 192,               // 良好（正常数据）
    GOOD_LOCAL_OVERRIDE = 216 // 良好（本地覆盖，人工写入的值）
};

// TagDataPoint 数据点结构体
// 既包含位号的静态描述信息（名称、单位、类型、来源），也包含实时运行值
struct TagDataPoint
{
    std::string tag_name;       // 数据点名称（位号）
    std::string description;    // 数据点描述
    std::string unit;           // 数据点单位
    DataType data_type;         // 数据点类型
    double raw_value;           // 原始值（设备读到的未换算值）
    double scaled_value;        // 工程值（换算后的显示值）
    Quality quality;            // 数据点质量
    uint64_t timestamp_ms;      // 时间戳（毫秒）
    std::string source_device;  // 数据源设备
    uint16_t source_address;    // 数据源地址

    // 判断数据点是否有效（质量不低于"不确定"即视为有效）
    bool isValid() const
    {
        return quality >= Quality::UNCERTAIN;
    }
};

// DataConverter 数据转换工具类
class DataConverter
{
public:
    // 线性量程转换：把原始值按比例映射到工程值区间
    // 例：raw ∈ [raw_min, raw_max] 线性映射到 eng ∈ [eng_min, eng_max]
    static double linearScale(double raw, double raw_min, double raw_max,
        double eng_min, double eng_max)
    {
        if (raw_max <= raw_min) return eng_min;
        return eng_min + (raw - raw_min) * (eng_max - eng_min) / (raw_max - raw_min);
    }

    // 差压式流量计的开方转换：差压与流量呈平方关系，需开方
    // 例：差压 dp 归一化后开方，再映射到流量区间 [flow_min, flow_max]
    static double sqrtFlow(double dp, double dp_min, double dp_max,
        double flow_min, double flow_max)
    {
        if (dp <= dp_min) return flow_min;
        double normalized = (dp - dp_min) / (dp_max - dp_min);
        double flow_normalized = std::sqrt(normalized);
        return flow_min + flow_normalized * (flow_max - flow_min);
    }
};

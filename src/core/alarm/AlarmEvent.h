// ============================================================
// AlarmEvent.h — 报警事件定义
// ============================================================
//
// 定义 AlarmEvent 报警事件结构体：表示一条由报警规则触发后产生的
// 具体报警实例（区别于 AlarmRule 规则定义本身）。
//
// 一条报警规则（AlarmRule）在运行期可触发多次，每次触发都会生成
// 一个对应的 AlarmEvent，记录触发时间、触发值、当前状态及确认/
// 清除时间等生命周期信息。
// ============================================================

#pragma once

#include "AlarmRule.h"   // 依赖其中的 AlarmType / AlarmPriority / AlarmState 枚举
#include <string>        // std::string
#include <cstdint>       // uint64_t

struct AlarmEvent
{
    std::string event_id;        // 事件唯一标识
    std::string rule_id;         // 关联的报警规则 ID（对应 AlarmRule::rule_id）
    std::string tag_name;        // 关联的位号名（如 "TE-101"）
    std::string message;         // 报警消息（由规则消息模板填充后生成）
    AlarmType type;              // 报警类型（高限/低限/变化率等）
    AlarmPriority priority;      // 报警优先级（严重/高/中/低）
    AlarmState state;            // 当前报警状态（激活/确认/清除等）

    double trigger_value;        // 触发报警时的位号数值
    double limit_value;          // 触发时的阈值（规则限值）

    uint64_t trigger_time_ms;    // 触发时间（毫秒，Unix 纪元）
    uint64_t ack_time_ms = 0;    // 确认时间（毫秒，0 表示未确认）
    uint64_t clear_time_ms = 0;  // 清除时间（毫秒，0 表示未清除）
    uint64_t ack_clear_time_ms = 0; // 确认后清除时间（毫秒，0 表示无）

    // 判断报警是否仍处于“活动”状态：
    // 激活中（ACTIVE）或已确认但未清除（ACKNOWLEDGED）均视为活动。
    bool isActive() const
    {
        return state == AlarmState::ACTIVE ||
            state == AlarmState::ACKNOWLEDGED;
    }

    // 判断报警是否已“关闭”（恢复到未激活状态）
    bool isClosed() const
    {
        return state == AlarmState::INACTIVE;
    }
};

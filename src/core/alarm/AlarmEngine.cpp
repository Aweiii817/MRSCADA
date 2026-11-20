// ============================================================
// AlarmEngine.cpp — 报警引擎实现
// ============================================================
//
// 实现 AlarmEngine 报警引擎。核心流程：
//   1. 定时器周期触发 onEvaluateTick()，遍历所有启用的规则；
//   2. 对每条规则读取实时数据，经 isTriggered() 判断是否触发；
//   3. 根据触发状态与现有事件，创建新事件、保持现状或标记清除；
//   4. 通过信号广播状态变化，供界面订阅。
// ============================================================

#include "AlarmEngine.h"
#include "rtdb/RealTimeDB.h"     // 实时数据库（读取位号当前值）
#include "Logger.h"              // 日志工具
#include "Utility.h"             // 工具函数（generateId 生成 ID）
#include <nlohmann/json.hpp>     // JSON 解析库
#include <QDateTime>             // 获取当前时间戳
#include <cmath>                 // 数学函数

using Json = nlohmann::json;

// ------------------------------------------------------------
// instance() — 单例访问
// ------------------------------------------------------------
AlarmEngine& AlarmEngine::instance()
{
    static AlarmEngine inst;
    return inst;
}

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
AlarmEngine::AlarmEngine()
    : QObject(nullptr)
{
}

// ------------------------------------------------------------
// 析构函数
// ------------------------------------------------------------
AlarmEngine::~AlarmEngine()
{
    stop();   // 停止评估定时器
}

// ------------------------------------------------------------
// loadRulesFromJson() — 从 JSON 加载报警规则
// ------------------------------------------------------------
// 解析 JSON 中 "rules" 数组，逐条构造 AlarmRule 并加入 m_rules。
// 规则 ID 或位号名称为空的条目会被跳过。返回成功加载的规则数量。
// ------------------------------------------------------------
size_t AlarmEngine::loadRulesFromJson(const std::string& json_str)
{
    m_rules.clear();

    try {
        Json j = Json::parse(json_str);
        // 校验 "rules" 字段存在且为数组
        if (!j.contains("rules") || !j["rules"].is_array()) {
            LOG_ERROR("AlarmEngine: JSON missing 'rules' array");
            return 0;
        }

        for (const auto& item : j["rules"]) {
            AlarmRule rule;
            rule.rule_id = item.value("rule_id", "");
            rule.tag_name = item.value("tag_name", "");
            rule.type = static_cast<AlarmType>(item.value("type", 0));
            rule.limit = item.value("limit", 0.0);
            rule.hysteresis = item.value("hysteresis", 1.0);
            rule.priority = static_cast<AlarmPriority>(item.value("priority", 2));
            rule.message = item.value("message", "");
            rule.enabled = item.value("enabled", true);

            // 规则 ID 与位号名称为必填，缺失则跳过
            if (rule.rule_id.empty() || rule.tag_name.empty()) continue;

            m_rules.push_back(rule);
        }

        LOG_INFO("AlarmEngine: loaded {} rules", m_rules.size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("AlarmEngine: JSON parse failed - {}", e.what());
    }

    return m_rules.size();
}

// ------------------------------------------------------------
// start() — 启动评估
// ------------------------------------------------------------
// 懒创建定时器并启动，以固定间隔触发评估。
// ------------------------------------------------------------
void AlarmEngine::start(int interval_ms)
{
    if (m_running) return;

    // 懒创建定时器并连接槽函数
    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout,
            this, &AlarmEngine::onEvaluateTick);
    }
    m_timer->start(interval_ms);
    m_running = true;

    LOG_INFO("AlarmEngine: started (interval={}ms)", interval_ms);
}

// ------------------------------------------------------------
// stop() — 停止评估
// ------------------------------------------------------------
void AlarmEngine::stop()
{
    if (m_timer) m_timer->stop();
    m_running = false;
}

// ------------------------------------------------------------
// onEvaluateTick() — 评估定时器槽函数
// ------------------------------------------------------------
// 每个评估周期遍历所有规则，跳过未启用的规则，逐条评估。
// ------------------------------------------------------------
void AlarmEngine::onEvaluateTick()
{
    for (const auto& rule : m_rules) {
        if (!rule.enabled) continue;
        evaluateRule(rule);
    }
}

// ------------------------------------------------------------
// evaluateRule() — 评估单条规则
// ------------------------------------------------------------
// 核心评估逻辑：
//   1. 读取该位号当前值，无数据则跳过；
//   2. 通信故障报警（COMM_FAULT）单独处理（依据数据质量）；
//   3. 其余类型调用 isTriggered() 判断是否触发；
//   4. 触发且无活动事件 → 创建新事件并置为 ACTIVE；
//   5. 触发且已有活动事件 → 保持现状（避免重复报警）；
//   6. 不触发但有活动事件 → 标记为 CLEARED_UNACK（恢复待确认）。
// ------------------------------------------------------------
void AlarmEngine::evaluateRule(const AlarmRule& rule)
{
    // ---- 1. 读取当前值 ----
    TagDataPoint point;
    if (!RealTimeDB::instance().getTagValue(rule.tag_name, point)) {
        return;   // 还没有数据，跳过本次评估
    }

    // ---- 2. 特殊处理：通信故障报警 ----
    // 数据质量 BAD 表示设备离线/传感器故障
    if (rule.type == AlarmType::COMM_FAULT) {
        bool fault = (point.quality == Quality::BAD);
        if (fault) {
            // 通信故障 → 若无活动事件则创建新报警
            if (!findActiveEventByRule(rule.rule_id)) {
                AlarmEvent ev;
                ev.event_id = Utility::generateId();
                ev.rule_id = rule.rule_id;
                ev.tag_name = rule.tag_name;
                ev.message = rule.message.empty()
                    ? "通信故障：" + rule.tag_name : rule.message;
                ev.type = rule.type;
                ev.priority = rule.priority;
                ev.state = AlarmState::ACTIVE;
                ev.trigger_value = point.scaled_value;
                ev.limit_value = rule.limit;
                ev.trigger_time_ms = QDateTime::currentMSecsSinceEpoch();

                m_events.push_back(ev);
                m_active_by_rule[rule.rule_id] = &m_events.back();

                emit alarmActivated(QString::fromStdString(ev.event_id));
                emit activeAlarmCountChanged(activeAlarmCount());
                LOG_WARN("AlarmEngine: COMM_FAULT on {}", rule.tag_name);
            }
        }
        else {
            // 通信恢复 → 标记为恢复待确认
            AlarmEvent* ev = findActiveEventByRule(rule.rule_id);
            if (ev && ev->state != AlarmState::CLEARED_UNACK) {
                ev->state = AlarmState::CLEARED_UNACK;
                ev->clear_time_ms = QDateTime::currentMSecsSinceEpoch();
                emit alarmCleared(QString::fromStdString(ev->event_id));
            }
        }
        return;
    }

    // ---- 3. 判断是否触发 ----
    bool triggered = isTriggered(rule, point.scaled_value);

    // 查找该规则下“活动”的事件
    AlarmEvent* active_ev = findActiveEventByRule(rule.rule_id);

    if (triggered) {
        // ---- 报警条件满足 ----
        if (!active_ev) {
            // 没有活动事件 → 创建新报警
            AlarmEvent ev;
            ev.event_id = Utility::generateId();
            ev.rule_id = rule.rule_id;
            ev.tag_name = rule.tag_name;
            ev.message = rule.message.empty()
                ? rule.tag_name + " 报警" : rule.message;
            ev.type = rule.type;
            ev.priority = rule.priority;
            ev.state = AlarmState::ACTIVE;
            ev.trigger_value = point.scaled_value;
            ev.limit_value = rule.limit;
            ev.trigger_time_ms = QDateTime::currentMSecsSinceEpoch();

            m_events.push_back(ev);
            m_active_by_rule[rule.rule_id] = &m_events.back();

            emit alarmActivated(QString::fromStdString(ev.event_id));
            emit activeAlarmCountChanged(activeAlarmCount());

            LOG_WARN("AlarmEngine: {} triggered on {} (value={:.1f}, limit={:.1f})",
                ev.message, rule.tag_name, point.scaled_value, rule.limit);
        }
        // 已有活动事件 → 什么都不做（避免重复报警）
    }
    else {
        // ---- 报警条件不满足 ----
        if (active_ev && active_ev->state != AlarmState::CLEARED_UNACK) {
            // 有活动事件但现在条件不满足了 → 进入“恢复待确认”
            active_ev->state = AlarmState::CLEARED_UNACK;
            active_ev->clear_time_ms = QDateTime::currentMSecsSinceEpoch();
            emit alarmCleared(QString::fromStdString(active_ev->event_id));
            LOG_INFO("AlarmEngine: {} cleared on {}", active_ev->message,
                active_ev->tag_name);
        }
    }
}

// ------------------------------------------------------------
// isTriggered() — 判断规则是否触发
// ------------------------------------------------------------
// 根据报警类型对当前值进行判定：
//   HIGH / HIGH_HIGH：值大于限值；
//   LOW / LOW_LOW：值小于限值；
//   DIGITAL_ON：值大于 0.5（视为开）；
//   DIGITAL_OFF：值小于等于 0.5（视为关）；
//   变化率类型（RATE_UP/RATE_DOWN）：本阶段暂未实现，恒返回 false。
// ------------------------------------------------------------
bool AlarmEngine::isTriggered(const AlarmRule& rule, double value) const
{
    switch (rule.type) {
    case AlarmType::HIGH:
        return value > rule.limit;
    case AlarmType::HIGH_HIGH:
        return value > rule.limit;
    case AlarmType::LOW:
        return value < rule.limit;
    case AlarmType::LOW_LOW:
        return value < rule.limit;
    case AlarmType::DIGITAL_ON:
        return value > 0.5;   // 数字量为 1
    case AlarmType::DIGITAL_OFF:
        return value <= 0.5;  // 数字量为 0
    default:
        // 变化率报警需要历史数据，简化处理：本阶段暂不实现
        return false;
    }
}

// ------------------------------------------------------------
// findActiveEventByRule() — 查找规则的活动事件
// ------------------------------------------------------------
// 从 m_active_by_rule 映射中查找规则对应的活动事件指针。
// 若事件已关闭（INACTIVE），则从映射中移除并返回 nullptr，
// 保证映射中只保留真正“活动”的事件。
// ------------------------------------------------------------
AlarmEvent* AlarmEngine::findActiveEventByRule(const std::string& rule_id)
{
    auto it = m_active_by_rule.find(rule_id);
    if (it != m_active_by_rule.end()) {
        // 若事件已经关闭，从激活表移除
        if (it->second->isClosed()) {
            m_active_by_rule.erase(it);
            return nullptr;
        }
        return it->second;
    }
    return nullptr;
}

// ------------------------------------------------------------
// acknowledge() — 确认报警
// ------------------------------------------------------------
// 将状态为 ACTIVE 的事件转为 ACKNOWLEDGED，记录确认时间。
// 仅 ACTIVE 状态可被确认；返回是否成功。
// ------------------------------------------------------------
bool AlarmEngine::acknowledge(const std::string& event_id)
{
    // 线性遍历查找事件（事件数量不多，线性查找足够；
    // 生产环境可改用哈希表）
    for (auto& ev : m_events) {
        if (ev.event_id == event_id) {
            if (ev.state == AlarmState::ACTIVE) {
                ev.state = AlarmState::ACKNOWLEDGED;
                ev.ack_time_ms = QDateTime::currentMSecsSinceEpoch();
                emit alarmAcknowledged(QString::fromStdString(event_id));
                LOG_INFO("AlarmEngine: acknowledged {}", event_id);
                return true;
            }
            break;
        }
    }
    return false;
}

// ------------------------------------------------------------
// acknowledgeClear() — 确认“清除待确认”报警
// ------------------------------------------------------------
// 将状态为 CLEARED_UNACK 的事件转为 INACTIVE（彻底关闭），
// 记录确认后清除时间，并从激活表移除。返回是否成功。
// ------------------------------------------------------------
bool AlarmEngine::acknowledgeClear(const std::string& event_id)
{
    for (auto& ev : m_events) {
        if (ev.event_id == event_id) {
            if (ev.state == AlarmState::CLEARED_UNACK) {
                // 恢复确认 → 事件彻底关闭
                ev.state = AlarmState::INACTIVE;
                ev.ack_clear_time_ms = QDateTime::currentMSecsSinceEpoch();

                // 从激活表移除
                m_active_by_rule.erase(ev.rule_id);

                emit alarmClosed(QString::fromStdString(event_id));
                emit activeAlarmCountChanged(activeAlarmCount());
                LOG_INFO("AlarmEngine: cleared-ack {}", event_id);
                return true;
            }
            break;
        }
    }
    return false;
}

// ------------------------------------------------------------
// getActiveAlarms() — 获取所有活动报警
// ------------------------------------------------------------
std::vector<AlarmEvent> AlarmEngine::getActiveAlarms() const
{
    std::vector<AlarmEvent> result;
    for (const auto& ev : m_events) {
        if (ev.isActive()) {
            result.push_back(ev);
        }
    }
    return result;
}

// ------------------------------------------------------------
// getAllEvents() — 获取全部报警事件（含历史）
// ------------------------------------------------------------
std::vector<AlarmEvent> AlarmEngine::getAllEvents() const
{
    return m_events;
}

// ------------------------------------------------------------
// activeAlarmCount() — 获取活动报警数量
// ------------------------------------------------------------
int AlarmEngine::activeAlarmCount() const
{
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.isActive()) ++count;
    }
    return count;
}

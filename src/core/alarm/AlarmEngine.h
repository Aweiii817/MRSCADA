// ============================================================
// AlarmEngine.h — 报警引擎
// ============================================================
//
// AlarmEngine 是报警系统的核心引擎（单例），继承自 QObject。
// 负责：
//   1. 从 JSON 加载报警规则（AlarmRule）；
//   2. 以固定周期（定时器）遍历规则，读取实时数据并判定是否触发；
//   3. 管理报警事件（AlarmEvent）的生命周期：激活 → 确认 → 清除/关闭；
//   4. 通过 Qt 信号向界面（报警管理页等）广播报警状态变化。
//
// 报警生命周期状态机：
//   未激活(INACTIVE) ──触发──> 激活(ACTIVE) ──人工确认──> 已确认(ACKNOWLEDGED)
//   激活(ACTIVE) ──条件消失──> 清除待确认(CLEARED_UNACK) ──确认──> 未激活(INACTIVE)
// ============================================================

#pragma once

#include <QObject>            // QObject 基类（信号槽）
#include <QTimer>             // 评估定时器
#include <vector>             // std::vector（规则/事件容器）
#include <unordered_map>      // std::unordered_map（规则→激活事件映射）
#include <memory>             // std::unique_ptr 等

#include "AlarmRule.h"        // 报警规则与枚举定义
#include "AlarmEvent.h"       // 报警事件结构体

class AlarmEngine : public QObject
{
    Q_OBJECT

public:
    // 单例访问接口（Meyers 单例，线程安全）
    static AlarmEngine& instance();

    // 从 JSON 字符串加载报警规则，返回成功加载的规则数量
    size_t loadRulesFromJson(const std::string& json_str);

    // 启动报警评估（默认每 200ms 评估一次）
    void start(int interval_ms = 200);

    // 停止报警评估
    void stop();

    // 确认一条报警（ACTIVE → ACKNOWLEDGED），返回是否成功
    bool acknowledge(const std::string& event_id);

    // 确认一条“清除待确认”报警（CLEARED_UNACK → INACTIVE），返回是否成功
    bool acknowledgeClear(const std::string& event_id);

    // 获取所有当前处于活动状态的报警事件
    std::vector<AlarmEvent> getActiveAlarms() const;

    // 获取全部报警事件（含历史）
    std::vector<AlarmEvent> getAllEvents() const;

    // 获取当前活动报警数量
    int activeAlarmCount() const;

signals:
    // 报警被激活（新报警产生）
    void alarmActivated(const QString& event_id);

    // 报警被人工确认
    void alarmAcknowledged(const QString& event_id);

    // 报警条件消失（进入清除待确认状态）
    void alarmCleared(const QString& event_id);

    // 报警彻底关闭（清除后确认）
    void alarmClosed(const QString& event_id);

    // 活动报警数量发生变化
    void activeAlarmCountChanged(int count);

private slots:
    // 评估定时器超时槽函数：遍历所有规则并评估
    void onEvaluateTick();

private:
    // 构造函数与析构函数私有化，仅通过 instance() 获取实例
    AlarmEngine();
    ~AlarmEngine() override;

    // 评估单条规则：读取数据、判断触发、更新事件状态
    void evaluateRule(const AlarmRule& rule);

    // 判断规则在当前数值下是否触发（纯函数，不修改状态）
    bool isTriggered(const AlarmRule& rule, double value) const;

    // 查找某规则下当前“活动”的报警事件（若无则返回 nullptr）
    AlarmEvent* findActiveEventByRule(const std::string& rule_id);

    QTimer* m_timer = nullptr;      // 评估定时器
    std::vector<AlarmRule> m_rules; // 已加载的报警规则列表
    std::vector<AlarmEvent> m_events; // 全部报警事件（含历史）

    // 规则 ID → 当前活动事件的指针映射，用于快速查找，
    // 避免每次评估都线性扫描全部事件
    std::unordered_map<std::string, AlarmEvent*> m_active_by_rule;

    bool m_running = false;         // 引擎是否正在运行
};

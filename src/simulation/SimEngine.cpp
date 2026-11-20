// ============================================================
// SimEngine.cpp — 仿真引擎实现
// ============================================================
//
// 实现仿真引擎的启停、定时推进、状态发布与阶段切换逻辑。
// 运行流程：start() 启动定时器 → 每个周期 onTimerTick() 触发，
// 依次推进模型、检查阶段切换、发布状态到实时数据库。
// ============================================================
#include "SimEngine.h"
#include "rtdb/RealTimeDB.h"   // 实时数据库（状态发布目标）
#include "Logger.h"
#include <QDateTime>           // 获取当前时间戳（毫秒）

// 构造函数：创建定时器并连接其超时信号到仿真步长槽函数
SimEngine::SimEngine(QObject* parent)
    : QObject(parent)
{
	// 创建定时器，用于定时触发仿真步长（定时器父对象为本对象，自动管理生命周期）
    m_timer = new QTimer(this);
	// 连接定时器的超时信号到仿真步长处理槽函数
    connect(m_timer, &QTimer::timeout,
        this, &SimEngine::onTimerTick);
}

// 启动仿真引擎
void SimEngine::start(int interval_ms)
{
    // 已在运行则直接返回，避免重复启动
    if (m_running) {
        LOG_WARN("SimEngine: already running");
        return;
    }

    // 设置运行状态、换算仿真步长（毫秒→秒），并重置模型到进料阶段
    m_running = true;
    m_dt_seconds = interval_ms / 1000.0;
    m_model.reset();
    m_model.setStage(ProcessStage::FEEDING);

	// 启动定时器，按指定间隔周期性触发仿真步长
    m_timer->start(interval_ms);

    LOG_INFO("SimEngine: started (interval={}ms)", interval_ms);
}

// 停止仿真引擎
void SimEngine::stop()
{
    // 未运行则直接返回
    if (!m_running) return;

    m_timer->stop();      // 停止定时器
    m_running = false;    // 清除运行标志

    LOG_INFO("SimEngine: stopped");
}

// 检查仿真引擎是否正在运行
bool SimEngine::isRunning() const
{
    return m_running;
}

// 获取仿真模型（可读写版本）
SimReactorModel& SimEngine::model()
{
    return m_model;
}

// 获取仿真模型（只读版本）
const SimReactorModel& SimEngine::model() const
{
    return m_model;
}

// 定时器触发槽函数：每个仿真周期执行一次完整推进流程
void SimEngine::onTimerTick()
{
	// 1. 推进仿真模型一个时间步长
    m_model.step(m_dt_seconds);
	// 2. 检查当前工艺阶段是否需要切换到下一个阶段
    checkStageTransition();
	// 3. 将反应釜的状态值发布到 RealTimeDB
    publishState();

    // 4. 发出仿真步长信号，通知外部（携带当前毫秒时间戳）
    emit simulationStep(QDateTime::currentMSecsSinceEpoch());
}

// 将反应釜的状态值发布到 RealTimeDB（含噪声叠加与阀门离散化）
void SimEngine::publishState()
{
    auto& rtdb = RealTimeDB::instance();
    const auto& s = m_model.getState();

    // 发布反应釜核心过程变量（叠加测量噪声模拟真实传感器）
    rtdb.updateTagValue("TE-101", m_model.applyNoise(s.temperature));
    rtdb.updateTagValue("TE-102", m_model.applyNoise(s.jacket_temp));
    rtdb.updateTagValue("PT-101", m_model.applyNoise(s.pressure));
    rtdb.updateTagValue("LT-101", m_model.applyNoise(s.level));
    rtdb.updateTagValue("FT-101", m_model.applyNoise(s.feed_a_flow));
    rtdb.updateTagValue("FT-102", m_model.applyNoise(s.feed_b_flow));
    rtdb.updateTagValue("TT-101", m_model.applyNoise(s.agitator_speed));

    // 发布原料罐 A/B 参数（液位、温度、压力）
    rtdb.updateTagValue("LT-201", m_model.applyNoise(s.tank_a_level));
    rtdb.updateTagValue("TE-201", m_model.applyNoise(s.tank_a_temp));
    rtdb.updateTagValue("PT-201", m_model.applyNoise(s.tank_a_pressure));
    rtdb.updateTagValue("LT-202", m_model.applyNoise(s.tank_b_level));
    rtdb.updateTagValue("TE-202", m_model.applyNoise(s.tank_b_temp));
    rtdb.updateTagValue("PT-202", m_model.applyNoise(s.tank_b_pressure));

    // 阀门状态离散化：开度 > 0.5 视为全开(1.0)，否则全关(0.0)
    // 涵盖进料阀、出料阀、蒸汽调节阀、冷却水调节阀与泄压阀
    double hv101_val = (s.valve_hv101 > 0.5) ? 1.0 : 0.0;
    double hv103_val = (s.valve_hv103 > 0.5) ? 1.0 : 0.0;

    rtdb.setTagValue("HV-101", hv101_val);
    rtdb.setTagValue("HV-102", (s.valve_hv102 > 0.5) ? 1.0 : 0.0);
    rtdb.setTagValue("HV-103", hv103_val);
    rtdb.setTagValue("TV-101", (s.valve_tv101 > 0.5) ? 1.0 : 0.0);  // 蒸汽调节阀
    rtdb.setTagValue("TV-102", (s.valve_tv102 > 0.5) ? 1.0 : 0.0);  // 冷却水调节阀
    rtdb.setTagValue("PV-101", (s.valve_pv101 > 0.5) ? 1.0 : 0.0);  // 泄压阀
}

// 检查当前工艺阶段是否需要切换到下一个阶段
// 根据当前阶段、液位/温度/计时等条件判断切换时机并触发切换
void SimEngine::checkStageTransition()
{
    double elapsed = m_model.stageElapsedTime();   // 当前阶段已运行时间
    const auto& s = m_model.getState();            // 当前状态快照

    switch (m_model.currentStage()) {
    case ProcessStage::FEEDING:
        // 液位达到目标值（98%）→ 关闭进料阀、开蒸汽阀，切换加热
        if (s.level >= m_model.params().setpoint_level * 0.98) {
            m_model.setValve("HV-101", 0.0);
            m_model.setValve("TV-101", 0.8);
            m_model.setStage(ProcessStage::HEATING);
            LOG_INFO("SimEngine: FEEDING → HEATING (level={:.1f})", s.level);
        }
        break;

    case ProcessStage::HEATING:
		// 温度达到设定值（97%）→ 调小蒸汽阀，切换反应
        if (s.temperature >= m_model.params().setpoint_temp * 0.97) {
            m_model.setValve("TV-101", 0.3);
            m_model.setStage(ProcessStage::REACTING);
            LOG_INFO("SimEngine: HEATING → REACTING (temp={:.1f})", s.temperature);
        }
        break;

    case ProcessStage::REACTING:
		// 反应阶段持续 60 秒 → 关蒸汽阀、开冷却水阀，切换冷却
        if (elapsed >= 60.0) {
            m_model.setValve("TV-101", 0.0);
            m_model.setValve("TV-102", 0.7);
            m_model.setStage(ProcessStage::COOLING);
            LOG_INFO("SimEngine: REACTING → COOLING (elapsed={:.0f}s)", elapsed);
        }
        break;

    case ProcessStage::COOLING:
		// 温度降到 50°C 以下 → 关冷却水阀、开出料阀，切换出料
        if (s.temperature <= 50.0) {
            m_model.setValve("TV-102", 0.0);
            m_model.setValve("HV-103", 1.0);
            m_model.setStage(ProcessStage::DISCHARGING);
            LOG_INFO("SimEngine: COOLING → DISCHARGING (temp={:.1f})", s.temperature);
        }
        break;

    case ProcessStage::DISCHARGING:
		// 液位降到 5% 以下 → 关出料阀，切换待机（批次完成）
        if (s.level <= 5.0) {
            m_model.setValve("HV-103", 0.0);
            m_model.setStage(ProcessStage::IDLE);
            LOG_INFO("SimEngine: DISCHARGING → IDLE (batch complete)");
        }
        break;

    case ProcessStage::IDLE:
        // 待机阶段：等待外部触发（如配方执行）
        break;

    default:
        break;
    }
}

// ============================================================
// SimReactorModel.cpp — 反应釜模型实现
// ============================================================
//
// 实现反应釜数学模型的具体逻辑。核心思想：
//   1. step() 按时间步长依序推进各过程变量（流量→液位→温度→压力→原料罐）；
//   2. 每个过程变量采用"一阶惯性"近似，并受当前工艺阶段与阀门开度控制；
//   3. 所有变量在计算后均做限幅（clamp），避免数值溢出到不合理区间。
// ============================================================
#include "SimReactorModel.h"
#include "Logger.h"
#include <algorithm>   // std::clamp
#include <cmath>       // 数学函数（本文件未直接使用，保留以备扩展）

// 构造函数：初始化工艺阶段、计时、随机数生成器，并重置状态
SimReactorModel::SimReactorModel()
    : m_stage(ProcessStage::IDLE)               // 初始为待机阶段
    , m_stage_elapsed(0.0)                      // 阶段计时归零
    , m_rng(std::random_device{}())             // 用随机设备播种随机数引擎
    , m_noise_dist(0.0, 1.0)                    // 标准正态分布（均值0，标准差1）
{
    reset();
}

// 设置仿真参数（整组替换）
void SimReactorModel::setParams(const ReactorParams& params)
{
    m_params = params;
}

// 获取仿真参数（只读）
const ReactorParams& SimReactorModel::params() const
{
    return m_params;
}

// 获取当前状态（只读）
const ReactorState& SimReactorModel::getState() const
{
    return m_state;
}

// 设置当前工艺阶段：阶段发生变化时记录日志并重置该阶段计时
void SimReactorModel::setStage(ProcessStage stage)
{
    if (m_stage != stage) {
        LOG_INFO("SimReactor: stage transition {} → {}",
            static_cast<int>(m_stage),
            static_cast<int>(stage));
        m_stage = stage;
        m_stage_elapsed = 0.0;   // 新阶段从零开始计时
    }
}

// 获取当前工艺阶段
ProcessStage SimReactorModel::currentStage() const
{
    return m_stage;
}

// 获取当前工艺阶段已运行时间（秒）
double SimReactorModel::stageElapsedTime() const
{
    return m_stage_elapsed;
}

// 设置阀门开度（0.0 = 全关，1.0 = 全开），按位号匹配对应阀门
void SimReactorModel::setValve(const std::string& tag_name, double position)
{
	// 将开度限制在 0.0 到 1.0 之间，防止非法输入
    double pos = std::clamp(position, 0.0, 1.0);

    // 按位号分发到对应的阀门变量，未知位号记录警告
    if (tag_name == "HV-101") m_state.valve_hv101 = pos;
    else if (tag_name == "HV-102") m_state.valve_hv102 = pos;
    else if (tag_name == "HV-103") m_state.valve_hv103 = pos;
    else if (tag_name == "TV-101") m_state.valve_tv101 = pos;
    else if (tag_name == "TV-102") m_state.valve_tv102 = pos;
    else if (tag_name == "PV-101") m_state.valve_pv101 = pos;
    else LOG_WARN("SimReactor: unknown valve '{}'", tag_name);
}

// 推进仿真模型一个时间步长（单位：秒）
// 依序更新流量、液位、温度、压力、原料罐状态
void SimReactorModel::step(double dt_seconds)
{
	// 将时间步长限制在合理范围内，避免数值不稳定
    if (dt_seconds <= 0.0 || dt_seconds > 10.0) return;

    m_stage_elapsed += dt_seconds;   // 累加当前阶段已运行时间

	// 根据推进的时间步长依次更新各个状态变量
    updateFlow(dt_seconds);
    updateLevel(dt_seconds);
    updateTemperature(dt_seconds);
    updatePressure(dt_seconds);
    updateTanks(dt_seconds);
}

// 更新温度状态：根据工艺阶段计算釜内温度与夹套温度的变化
void SimReactorModel::updateTemperature(double dt)
{
    double target_temp = m_params.setpoint_temp;   // 目标温度（设定值）
    double tau = m_params.heat_time_const;         // 加热时间常数

	// 根据当前工艺阶段计算温度变化
    switch (m_stage) {
    // 加热阶段：温度向设定值一阶逼近，夹套温度高于釜内温度
    case ProcessStage::HEATING:
        if (m_state.temperature < target_temp) {
            double rate = (target_temp - m_state.temperature)
                * m_state.valve_tv101 / tau;   // 升温速率受蒸汽阀开度控制
            m_state.temperature += rate * dt;
        }

        m_state.jacket_temp = m_state.temperature + 30.0
            * m_state.valve_tv101;   // 夹套温度比釜内高，随蒸汽阀开度增加
        break;
	// 反应阶段：温度维持在设定值附近（比例修正），夹套略高
    case ProcessStage::REACTING:
    {
        double error = target_temp - m_state.temperature;   // 温度偏差
        double correction = error * 0.1 * dt;               // 比例修正量
        m_state.temperature += correction;
    }
    m_state.jacket_temp = m_state.temperature + 10.0;
    break;
	// 冷却阶段：温度向环境温度下降，夹套温度低于釜内温度
    case ProcessStage::COOLING:
    {
        double cool_tau = m_params.cool_time_const;   // 冷却时间常数
        double ambient = 25.0;                        // 环境温度
        double rate = (ambient - m_state.temperature)
            * m_state.valve_tv102 / cool_tau;         // 降温速率受冷却水阀开度控制
        m_state.temperature += rate * dt;
    }
    m_state.jacket_temp = m_state.temperature - 20.0
        * m_state.valve_tv102;   // 夹套温度比釜内低，随冷却水阀开度增加
    break;
	// 其他阶段：温度自然回到环境温度，夹套温度与釜内温度相同
    default:
    {
        double ambient = 25.0;
        double rate = (ambient - m_state.temperature) / 600.0;  // 缓慢自然冷却
        m_state.temperature += rate * dt;
    }
    m_state.jacket_temp = m_state.temperature;
    break;
    }

	// 将温度和夹套温度限制在合理范围内，避免数值溢出
    m_state.temperature = std::clamp(m_state.temperature, 25.0, m_params.max_temp);
    m_state.jacket_temp = std::clamp(m_state.jacket_temp, 20.0, m_params.max_temp + 50.0);
}

// 更新液位状态：进料阶段上升，出料阶段下降，其他阶段保持不变
void SimReactorModel::updateLevel(double dt)
{
    switch (m_stage) {
	// 进料阶段：液位按进料速率上升，受进料阀开度控制
    case ProcessStage::FEEDING:
        if (m_state.level < m_params.setpoint_level) {
            m_state.level += m_params.feed_rate
                * m_state.valve_hv101 * dt;
        }
        break;
	// 出料阶段：液位按出料速率下降，受出料阀开度控制
    case ProcessStage::DISCHARGING:
        if (m_state.level > 0.0) {
            m_state.level -= m_params.discharge_rate
                * m_state.valve_hv103 * dt;
        }
        break;
	// 其他阶段：液位保持不变
    default:
        break;
    }

	// 将液位限制在 0% 到 100% 之间，避免数值溢出
    m_state.level = std::clamp(m_state.level, 0.0, 100.0);
}

// 更新压力状态：压力由温度决定（理想气体近似），并受泄压阀影响
void SimReactorModel::updatePressure(double dt)
{
	// 基础压力（bar），假设在 25°C 时的压力为 0.1 bar（常压）
    double base_pressure = 0.1;
	// 温度对压力的贡献，假设每升高 1°C，压力增加 0.008 bar
    double temp_contribution = (m_state.temperature - 25.0)
        * 0.008;

    double target_pressure = base_pressure + temp_contribution;
	// 泄压阀对压力的影响：开度越大，压力下降越快
    if (m_state.valve_pv101 > 0.01) {
        target_pressure -= m_state.valve_pv101 * 2.0 * dt;
    }

    // 一阶惯性逼近目标压力（时间常数约 5 秒）
    m_state.pressure += (target_pressure - m_state.pressure) * dt / 5.0;
	// 将压力限制在 0 到最大压力之间，避免数值溢出
    m_state.pressure = std::clamp(m_state.pressure, 0.0, m_params.max_pressure);
}

// 更新流量状态：根据阀门开度计算进料流量，并按阶段设置搅拌转速
void SimReactorModel::updateFlow(double dt)
{
	// 根据阀门开度计算进料流量（进料B按 0.6 比例系数）
    m_state.feed_a_flow = m_state.valve_hv101 * m_params.valve_full_flow;
    m_state.feed_b_flow = m_state.valve_hv102 * m_params.valve_full_flow * 0.6;

	// 根据工艺阶段设置搅拌器转速
    if (m_stage == ProcessStage::REACTING) {
        m_state.agitator_speed = 300.0;    // 反应阶段高速搅拌
    }
    else if (m_stage == ProcessStage::HEATING ||
        m_stage == ProcessStage::COOLING) {
        m_state.agitator_speed = 100.0;    // 加热/冷却阶段低速搅拌（均匀传热）
    }
    else {
        m_state.agitator_speed = 0.0;      // 其他阶段停转
    }
}

// 更新原料罐状态（液位随进料下降，温度压力接近环境）
void SimReactorModel::updateTanks(double dt)
{
    // 原料罐液位随对应进料阀开启而下降（与反应釜进料速率一致）
    if (m_state.valve_hv101 > 0.01 && m_state.tank_a_level > 0.0) {
        m_state.tank_a_level -= m_params.feed_rate
            * m_state.valve_hv101 * dt;
    }
    if (m_state.valve_hv102 > 0.01 && m_state.tank_b_level > 0.0) {
        m_state.tank_b_level -= m_params.feed_rate
            * m_state.valve_hv102 * dt;
    }
    m_state.tank_a_level = std::clamp(m_state.tank_a_level, 0.0, 100.0);
    m_state.tank_b_level = std::clamp(m_state.tank_b_level, 0.0, 100.0);

    // 原料罐温度缓慢趋近环境温度（25℃），压力基本保持常压
    double ambient = 25.0;
    m_state.tank_a_temp += (ambient - m_state.tank_a_temp) * 0.01 * dt;
    m_state.tank_b_temp += (ambient - m_state.tank_b_temp) * 0.01 * dt;
    m_state.tank_a_pressure = 0.1 + (m_state.tank_a_temp - 25.0) * 0.001;
    m_state.tank_b_pressure = 0.1 + (m_state.tank_b_temp - 25.0) * 0.001;
}

// 为仿真值添加传感器测量噪声（模拟真实仪表测量波动）
double SimReactorModel::applyNoise(double value) const
{
    // 零值不添加噪声（如停转的搅拌器、关闭的阀门）
    if (value == 0.0) return 0.0;

    // 噪声 = 正态分布随机数 × 噪声水平 × 当前值（比例噪声）
    double noise = m_noise_dist(m_rng) * m_params.noise_level * value;
    return value + noise;
}

// 重置仿真模型到初始状态
void SimReactorModel::reset()
{
    m_state.reset();                 // 重置所有过程变量
    m_stage = ProcessStage::IDLE;    // 回到待机阶段
    m_stage_elapsed = 0.0;           // 阶段计时归零
    LOG_INFO("SimReactor: reset to initial state");
}

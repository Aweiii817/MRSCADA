// ============================================================
// SimReactorModel.h — 反应釜数学模型
// ============================================================
//
// 本文件定义仿真模块的核心数学模型，用于在无真实 PLC / 仪表
// 接入时，模拟一个精细化工反应釜的生产过程。主要内容包括：
//   ReactorState    反应釜当前过程变量的快照（温度/压力/液位等）
//   ProcessStage    反应釜所处的工艺阶段（进料→加热→反应→冷却→出料）
//   ReactorParams   反应釜的仿真参数（设定值、时间常数、噪声水平等）
//   SimReactorModel 反应釜模型类，负责按时间步长推进各过程变量
//
// 模型采用"一阶惯性 + 过程阶段切换"的简化方式近似真实工艺：
// 每个阶段有明确的进入/退出条件（由 SimEngine 判断），阶段内
// 各过程变量按微分方程（一阶近似）随时间演化。
// ============================================================
#pragma once

#include <string>    // std::string（阀门位号等字符串）
#include <random>    // 随机数生成器（模拟传感器测量噪声）

// ------------------------------------------------------------
// ReactorState — 反应釜的过程变量快照
//
// 保存某一时刻反应釜及原料罐的全部状态，供 UI 显示与实时数据库
// 发布使用。所有变量均采用"物理意义明确"的工程单位。
// ------------------------------------------------------------
struct ReactorState
{
	double temperature;    // 釜内温度 (°C)
	double jacket_temp;    // 夹套温度 (°C)
	double pressure;       // 釜内压力 (bar)
	double level;          // 釜内液位 (%)
	double feed_a_flow;    // 进料A流量 (L/h)
	double feed_b_flow;    // 进料B流量 (L/h)
	double agitator_speed; // 搅拌器转速 (rpm)

	// 原料罐 A/B 状态（用于工艺流程图原料罐显示）
	double tank_a_level;     // 原料罐A液位 (%)
	double tank_a_temp;      // 原料罐A温度 (°C)
	double tank_a_pressure;  // 原料罐A压力 (MPa)
	double tank_b_level;     // 原料罐B液位 (%)
	double tank_b_temp;      // 原料罐B温度 (°C)
	double tank_b_pressure;  // 原料罐B压力 (MPa)

    // 阀门状态（0.0 = 全关，1.0 = 全开）
	double valve_hv101;    // 进料A阀（HV-101）
	double valve_hv102;    // 进料B阀（HV-102）
	double valve_hv103;    // 出料阀（HV-103）
    double valve_tv101;    // 蒸汽调节阀（TV-101）
	double valve_tv102;    // 冷却水调节阀（TV-102）
    double valve_pv101;    // 泄压阀（PV-101）

	// 将全部状态重置为初始值（常温常压、空釜、阀门全关）
    void reset()
    {
        temperature = 25.0;      // 初始釜内温度 25°C
        jacket_temp = 25.0;      // 初始夹套温度 25°C
        pressure = 0.1;          // 初始压力 0.1 bar（常压）
        level = 0.0;             // 初始液位 0%（空釜）
        feed_a_flow = 0.0;       // 进料A流量归零
        feed_b_flow = 0.0;       // 进料B流量归零
        agitator_speed = 0.0;    // 搅拌器停转
        tank_a_level = 100.0;    // 原料罐A满罐
        tank_a_temp = 25.0;      // 原料罐A常温
        tank_a_pressure = 0.1;   // 原料罐A常压
        tank_b_level = 100.0;    // 原料罐B满罐
        tank_b_temp = 25.0;      // 原料罐B常温
        tank_b_pressure = 0.1;   // 原料罐B常压
        valve_hv101 = 0.0;       // 进料A阀全关
        valve_hv102 = 0.0;       // 进料B阀全关
        valve_hv103 = 0.0;       // 出料阀全关
        valve_tv101 = 0.0;       // 蒸汽调节阀全关
        valve_tv102 = 0.0;       // 冷却水调节阀全关
        valve_pv101 = 0.0;       // 泄压阀全关
    }
};

// ------------------------------------------------------------
// ProcessStage — 反应釜的工艺阶段（批次生产的完整流程）
// ------------------------------------------------------------
enum class ProcessStage
{
	IDLE,           // 待机状态：什么也不做，等待外部触发（如配方执行）
	FEEDING,        // 进料阶段：打开进料阀门，液位上升
	HEATING,        // 加热阶段：打开蒸汽阀门，温度上升
	REACTING,       // 反应阶段：保持温度和压力，搅拌反应
	COOLING,        // 冷却阶段：打开冷却水阀门，温度下降
	DISCHARGING     // 出料阶段：打开出料阀门，液位下降
};

// ------------------------------------------------------------
// ReactorParams — 反应釜的仿真参数设置
//
// 集中管理模型的物理参数与工艺设定值，便于统一调整。
// ------------------------------------------------------------
struct ReactorParams
{
    double volume = 5.0;                // 反应釜容积（m³）
	double max_temp = 300.0;            // 最大温度（°C），用于限幅
	double max_pressure = 2.5;          // 最大压力（MPa），用于限幅
	double heat_time_const = 120.0;     // 加热时间常数（秒），值越大升温越慢
	double cool_time_const = 180.0;     // 冷却时间常数（秒），值越大降温越慢
	double noise_level = 0.005;         // 仿真噪声水平（±0.5%），叠加到输出

    // 手动设定值（可由上位机或配方下发的目标值）
	double setpoint_temp = 150.0;       // 设定温度（°C）
	double setpoint_level = 80.0;       // 设定液位（%）
	double feed_rate = 15.0;            // 进料速率（%液位/秒）
	double discharge_rate = 8.0;        // 出料速率（%液位/秒）

	double valve_full_flow = 500.0;     // 阀门全开流量（L/h），用于计算进料流量
};

// ------------------------------------------------------------
// SimReactorModel — 仿真反应釜模型
//
// 对外提供"状态查询 + 参数设置 + 阀门控制 + 时间步长推进"接口。
// SimEngine 每周期调用 step() 推进模型，再读取 getState() 发布。
// ------------------------------------------------------------
class SimReactorModel
{
public:
    SimReactorModel();            // 构造函数：初始化随机数并重置状态
    ~SimReactorModel() = default; // 默认析构

	// 设置仿真参数（整组替换）
    void setParams(const ReactorParams& params);
	// 获取仿真参数（只读）
    const ReactorParams& params() const;
	// 获取当前状态（只读）
    const ReactorState& getState() const;
	// 设置当前工艺阶段（阶段变化时会重置该阶段计时）
    void setStage(ProcessStage stage);
	// 获取当前工艺阶段
    ProcessStage currentStage() const;
	// 获取当前工艺阶段已运行时间（秒）
    double stageElapsedTime() const;
	// 设置阀门开度（0.0 = 全关，1.0 = 全开），按位号匹配对应阀门
    void setValve(const std::string& tag_name, double position);
	// 推进仿真模型一个时间步长（单位：秒）
    void step(double dt_seconds);
	// 重置仿真模型到初始状态
    void reset();
	// 为仿真值添加传感器测量噪声（零值不加噪）
    double applyNoise(double value) const;

private:
	// 更新各个过程变量（内部被 step() 依序调用）
    void updateTemperature(double dt);  // 更新釜内/夹套温度
    void updateLevel(double dt);        // 更新釜内液位
    void updatePressure(double dt);     // 更新釜内压力
    void updateFlow(double dt);         // 更新进料流量与搅拌转速
    void updateTanks(double dt);        // 更新原料罐状态（液位随进料下降）

	ReactorState m_state;       // 当前状态
	ReactorParams m_params;     // 仿真参数
	ProcessStage m_stage;       // 当前工艺阶段
	double m_stage_elapsed;     // 当前工艺阶段已运行时间（秒）

    mutable std::mt19937 m_rng;		// 随机数生成器（用于模拟传感器噪声）
	mutable std::normal_distribution<double> m_noise_dist;  // 正态分布噪声生成器
};

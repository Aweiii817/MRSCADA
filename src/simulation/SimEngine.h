// ============================================================
// SimEngine.h — 仿真引擎
// ============================================================
//
// SimEngine 是仿真模块的"心脏"：它用一个 QTimer 周期性地触发
// 仿真步长，在每次触发时做三件事：
//   1. 推进反应釜模型一个时间步长；
//   2. 检查工艺阶段是否需要切换（如进料→加热）；
//   3. 把模型状态发布到实时数据库（RealTimeDB）。
//
// 外部（SimPort / 配方执行器）通过 start()/stop() 控制启停，
// 通过 model() 访问模型以设置参数或阀门，通过 simulationStep
// 信号监听每次仿真更新。
// ============================================================
#pragma once

#include <QObject>             // 信号槽基类
#include <QTimer>              // 定时器（驱动仿真步长）
#include "SimReactorModel.h"   // 反应釜数学模型

// 仿真引擎类：定时推进模型并把结果发布到实时数据库
class SimEngine : public QObject
{
    Q_OBJECT

public:
    explicit SimEngine(QObject* parent = nullptr);  // 构造：创建并连接定时器
    ~SimEngine() override = default;                // 默认析构

	// 启动仿真引擎，interval_ms 为仿真步长（毫秒），默认 100ms
    void start(int interval_ms = 100);
	// 停止仿真引擎
    void stop();
	// 检查仿真引擎是否正在运行
    bool isRunning() const;
	// 获取仿真模型（可读写，用于设置参数/阀门）
    SimReactorModel& model();
    // 获取仿真模型（只读）
    const SimReactorModel& model() const;

signals:
	// 仿真每步长状态更新信号，携带当前时间戳（毫秒）
    void simulationStep(qint64 timestamp_ms);

private slots:
	// 定时器触发槽函数：每次触发时推进仿真模型一个时间步长
    void onTimerTick();

private:
    // 将反应釜的状态值发布到 RealTimeDB（含噪声叠加与阀门离散化）
    void publishState();
	// 检查当前工艺阶段是否需要切换到下一个阶段
    void checkStageTransition();

	SimReactorModel m_model;   // 仿真模型（反应釜数学模型实例）
	QTimer* m_timer;           // 定时器，用于定时触发仿真步长

    bool m_running = false;    // 运行状态标志
	double m_dt_seconds = 0.1; // 仿真步长（秒），由 start() 的毫秒参数换算而来
};

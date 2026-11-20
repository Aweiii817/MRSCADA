// ============================================================
// ProcessView.h — 工艺流程图视图
// ============================================================
//
// ProcessView 是工艺流程图页面，继承自 QGraphicsView，用于展示
// 反应釜系统的实时工艺流程。它在一个 QGraphicsScene 中组织以下图元：
//   - 两个原料罐（QGraphicsRectItem 矩形）
//   - 一台反应釜（ReactorItem）
//   - 两台进料泵（PumpItem）
//   - 六个阀门（ValveItem：进料阀、出料阀、蒸汽调节阀、冷却水调节阀、泄压阀）
//   - 若干连接管道（PipeItem，含蒸汽/冷却水/泄压管线）
//
// 视图内置两个定时器：
//   - m_data_timer：周期读取实时数据库（RealTimeDB），刷新各图元状态；
//   - m_anim_timer：周期推进流动/旋转动画（管道流动、泵叶片旋转）。
//
// 此外还支持鼠标滚轮缩放视图（0.3~5 倍），以及按住鼠标拖动平移场景。
// ============================================================

#pragma once

#include <QGraphicsView>   // QGraphicsView 基类
#include <QTimer>          // QTimer 定时器

// 前置声明，减少头文件依赖（具体类在 .cpp 中包含）
class QGraphicsScene;
class QResizeEvent;
class ReactorItem;
class TankItem;
class PipeItem;
class ValveItem;
class PumpItem;

class ProcessView : public QGraphicsView
{
    Q_OBJECT

public:
    // 构造函数：初始化场景、图元、布局与定时器
    explicit ProcessView(QWidget* parent = nullptr);

    // 获取当前工艺阶段文字（供监控页面参数区显示）
    QString currentStageText() const;

protected:
    // 鼠标滚轮事件：实现视图缩放
    void wheelEvent(QWheelEvent* event) override;

    // 视图尺寸变化：在自动适配模式下重新缩放场景，使图元居中且大小合适
    void resizeEvent(QResizeEvent* event) override;

private slots:
    // 定时刷新：从实时数据库读取数据并更新各图元状态
    void refreshData();

    // 定时推进动画：推进管道流动与泵叶片旋转
    void advanceAnimations();

private:
    // 初始化场景（创建 QGraphicsScene 并设为当前视图场景）
    void setupScene();

    // 创建所有图元对象并加入场景
    void createItems();

    // 布置各图元在场景中的位置，并自适应缩放视图
    void layoutItems();

    QGraphicsScene* m_scene;              // 图形场景（容纳所有图元）

    ReactorItem* m_reactor = nullptr;     // 反应釜
    TankItem* m_tank_a = nullptr;         // 原料罐 A
    TankItem* m_tank_b = nullptr;         // 原料罐 B

    PipeItem* m_pipe_feed_a = nullptr;    // 原料罐 A 进料管道
    PipeItem* m_pipe_feed_b = nullptr;    // 原料罐 B 进料管道
    PipeItem* m_pipe_inlet = nullptr;     // 反应釜进料管道
    PipeItem* m_pipe_outlet = nullptr;    // 反应釜出料管道
    PipeItem* m_pipe_steam = nullptr;     // 蒸汽管线（经蒸汽调节阀 TV-101）
    PipeItem* m_pipe_cooling = nullptr;   // 冷却水管线（经冷却水调节阀 TV-102）
    PipeItem* m_pipe_relief = nullptr;    // 泄压管线（经泄压阀 PV-101）

    ValveItem* m_valve_hv101 = nullptr;   // 进料阀 HV-101
    ValveItem* m_valve_hv102 = nullptr;   // 进料阀 HV-102
    ValveItem* m_valve_hv103 = nullptr;   // 出料阀 HV-103
    ValveItem* m_valve_tv101 = nullptr;   // 蒸汽调节阀 TV-101
    ValveItem* m_valve_tv102 = nullptr;   // 冷却水调节阀 TV-102
    ValveItem* m_valve_pv101 = nullptr;   // 泄压阀 PV-101

    PumpItem* m_pump_a = nullptr;         // 进料泵 P-101
    PumpItem* m_pump_b = nullptr;         // 进料泵 P-102

    QTimer* m_data_timer;                 // 数据刷新定时器
    QTimer* m_anim_timer;                 // 动画推进定时器

    QString m_stage_text;                 // 当前工艺阶段文字
    bool m_auto_fit = true;               // 是否自动适配视图（用户滚轮缩放后关闭）
};

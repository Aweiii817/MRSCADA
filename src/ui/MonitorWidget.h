// ============================================================
// MonitorWidget.h — 监控页面
//
// 将工艺流程图、仪表盘、数据监视、当前报警与当前配方信息整合到同一页面：
//   左半部分：上方工艺流程图 + 下方仪表盘；
//   右半部分：上方详细数据监视 + 中间当前报警信息 + 下方当前配方信息。
// 各部分使用 QSplitter 组织，可拖动调整比例。
// ============================================================
#pragma once

#include <QWidget>
#include "business/recipe/Recipe.h"

class ProcessView;
class DashboardWidget;
class DataTableWidget;
class QLabel;
class QTableWidget;
class QTableView;
class AlarmTableModel;
class QTimer;

class MonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorWidget(QWidget* parent = nullptr);

private slots:
    // 刷新当前配方信息显示
    void refreshRecipeInfo();

    // 刷新当前报警信息显示
    void refreshAlarms();

private:
    void setupUI();

    // 生成阶段设定值摘要文字
    static QString stageTargetText(const RecipeStage& stage);

    // 刷新实时参数显示（温度 / 压力 / 液位）
    void refreshParameters();

    ProcessView* m_process_view = nullptr;   // 工艺流程图
    DashboardWidget* m_dashboard = nullptr;  // 仪表盘
    DataTableWidget* m_data_table = nullptr; // 数据监视

    // 当前报警信息
    QTableView* m_alarm_view = nullptr;      // 报警表格视图
    AlarmTableModel* m_alarm_model = nullptr; // 报警表格模型
    QTimer* m_alarm_timer = nullptr;         // 报警刷新定时器

    // 实时参数显示（左侧工艺流程图上部）
    QLabel* m_param_temp = nullptr;          // 釜内温度
    QLabel* m_param_pressure = nullptr;      // 釜内压力
    QLabel* m_param_level = nullptr;         // 釜内液位
    QLabel* m_param_stage = nullptr;         // 工艺阶段

    QLabel* m_recipe_summary = nullptr;      // 配方概要信息
    QTableWidget* m_recipe_table = nullptr;  // 配方阶段列表
};

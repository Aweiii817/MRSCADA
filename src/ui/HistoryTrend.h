// ============================================================
// HistoryTrend.h — 历史趋势查询界面
// ============================================================
//
// HistoryTrend 是“历史趋势”页面，继承自 QWidget，用于按测点与
// 时间范围查询历史数据并绘制折线趋势图，同时支持导出 CSV。
//
// 界面组成：
//   - 查询表单：测点下拉框 + 开始/结束时间；
//   - 操作按钮：查询、导出 CSV；
//   - 折线趋势图：QChart + QLineSeries，X 轴为时间（QDateTimeAxis），
//     Y 轴为数值（QValueAxis）。
//
// 数据来源：HistoryDB（历史数据库单例），测点列表来自 TagManager。
// ============================================================

#pragma once

#include <QWidget>   // QWidget 基类

// 前置声明（Qt Charts 相关类型与界面控件），减少头文件依赖，
// 具体定义在 .cpp 中包含。
class QChart;
class QChartView;
class QLineSeries;
class QDateTimeAxis;
class QValueAxis;
class QComboBox;
class QDateTimeEdit;
class QPushButton;

class HistoryTrend : public QWidget
{
    Q_OBJECT

public:
    // 构造函数：初始化并构建界面
    explicit HistoryTrend(QWidget* parent = nullptr);
    // 析构函数（默认实现）
    ~HistoryTrend() override;

private slots:
    // “查询”按钮槽函数：查询历史数据并刷新趋势图
    void onQuery();

    // “导出 CSV”按钮槽函数：将查询结果导出为 CSV 文件
    void onExportCsv();

private:
    // 构建界面（创建控件、布局与信号槽连接）
    void setupUI();

    // ---- Qt Charts 相关对象 ----
    QChart* m_chart = nullptr;            // 图表对象
    QChartView* m_chart_view = nullptr;   // 图表视图（嵌入界面）
    QLineSeries* m_series = nullptr;      // 折线数据系列
    QDateTimeAxis* m_axis_x = nullptr;    // X 轴（时间轴）
    QValueAxis* m_axis_y = nullptr;       // Y 轴（数值轴）

    // ---- 界面控件 ----
    QComboBox* m_tag_combo;               // 测点下拉框
    QDateTimeEdit* m_start_edit;          // 开始时间编辑框
    QDateTimeEdit* m_end_edit;            // 结束时间编辑框
    QPushButton* m_query_btn;             // 查询按钮
    QPushButton* m_export_btn;            // 导出 CSV 按钮
};

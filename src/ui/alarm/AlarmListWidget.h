// ============================================================
// AlarmListWidget.h — 报警列表页面
// ============================================================
//
// AlarmListWidget 是“报警管理”页面，继承自 QWidget。它用一个
// QTableView 展示所有当前活动的报警，并提供“确认报警”“确认恢复”
// 两个操作按钮，以及顶部活动报警数量统计标签。
//
// 数据来源：AlarmEngine（报警引擎单例），通过 AlarmTableModel
// 作为表格数据模型。页面内置定时器，每 500ms 刷新一次显示。
// ============================================================

#pragma once

#include <QWidget>   // QWidget 基类
#include <QTimer>    // 刷新定时器

// 前置声明，减少头文件依赖
class QTableView;
class QSortFilterProxyModel;
class QLabel;
class QPushButton;
class AlarmTableModel;

class AlarmListWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数：构建界面并启动刷新定时器
    explicit AlarmListWidget(QWidget* parent = nullptr);

private slots:
    // 刷新定时器槽函数：更新统计标签与表格数据
    void refreshAlarms();

    // “确认报警”按钮槽函数：确认当前选中行的报警
    void onAcknowledge();

    // “确认恢复”按钮槽函数：确认当前选中行的恢复状态
    void onAcknowledgeClear();

private:
    // 构建界面（创建控件、布局与信号槽连接）
    void setupUI();

    QLabel* m_count_label;      // 活动报警数量统计标签
    QTableView* m_view;         // 报警表格视图
    AlarmTableModel* m_model;   // 表格数据模型
    QPushButton* m_ack_btn;     // “确认报警”按钮
    QPushButton* m_ack_clear_btn; // “确认恢复”按钮
    QTimer* m_refresh_timer;    // 刷新定时器
};

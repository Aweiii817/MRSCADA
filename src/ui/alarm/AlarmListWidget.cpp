// ============================================================
// AlarmListWidget.cpp — 报警列表页面实现
// ============================================================
//
// 实现 AlarmListWidget 报警管理页面：用表格展示所有活动报警，
// 顶部显示活动报警数量统计，底部提供“确认报警”“确认恢复”按钮。
// 通过内置定时器每 500ms 从报警引擎拉取最新活动报警并刷新显示。
// ============================================================

#include "AlarmListWidget.h"
#include "AlarmTableModel.h"             // 报警表格数据模型
#include "core/alarm/AlarmEngine.h"      // 报警引擎（数据来源）

#include <QVBoxLayout>                   // 垂直布局
#include <QHBoxLayout>                   // 水平布局
#include <QTableView>                    // 表格视图
#include <QLabel>                        // 标签
#include <QPushButton>                   // 按钮
#include <QHeaderView>                   // 表头视图
#include <QMessageBox>                   // 消息提示框

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 构建界面，并启动刷新定时器（每 500ms 刷新一次）。
// ------------------------------------------------------------
AlarmListWidget::AlarmListWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    // 创建刷新定时器，每 500ms 触发一次 refreshAlarms
    m_refresh_timer = new QTimer(this);
    connect(m_refresh_timer, &QTimer::timeout,
        this, &AlarmListWidget::refreshAlarms);
    m_refresh_timer->start(500);
}

// ------------------------------------------------------------
// setupUI() — 构建界面
// ------------------------------------------------------------
// 布局（从上到下）：
//   1. 顶部统计区：活动报警数量标签；
//   2. 表格区：QTableView 展示报警列表；
//   3. 底部按钮区：确认报警、确认恢复。
// ------------------------------------------------------------
void AlarmListWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    // ---- 顶部统计区 ----
    auto* top = new QHBoxLayout();
    m_count_label = new QLabel(QString::fromUtf8("活跃报警: 0 条"));
    m_count_label->setStyleSheet("font-size: 16px; font-weight: bold;");
    top->addWidget(m_count_label);
    top->addStretch();
    layout->addLayout(top);

    // ---- 表格 ----
    m_model = new AlarmTableModel(this);
    m_view = new QTableView();
    m_view->setModel(m_model);
    m_view->setAlternatingRowColors(true);                     // 交替行颜色
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows); // 整行选中
    m_view->setSelectionMode(QAbstractItemView::SingleSelection); // 单选
    m_view->horizontalHeader()->setStretchLastSection(true);   // 最后一列拉伸
    m_view->verticalHeader()->setVisible(false);               // 隐藏行号
    layout->addWidget(m_view, 1);

    // ---- 底部按钮 ----
    auto* buttons = new QHBoxLayout();

    m_ack_btn = new QPushButton(QString::fromUtf8("确认报警"));
    connect(m_ack_btn, &QPushButton::clicked,
        this, &AlarmListWidget::onAcknowledge);
    buttons->addWidget(m_ack_btn);

    m_ack_clear_btn = new QPushButton(QString::fromUtf8("确认恢复"));
    connect(m_ack_clear_btn, &QPushButton::clicked,
        this, &AlarmListWidget::onAcknowledgeClear);
    buttons->addWidget(m_ack_clear_btn);

    buttons->addStretch();
    layout->addLayout(buttons);
}

// ------------------------------------------------------------
// refreshAlarms() — 刷新显示
// ------------------------------------------------------------
// 从报警引擎获取活动报警数量与列表，更新统计标签与表格。
// ------------------------------------------------------------
void AlarmListWidget::refreshAlarms()
{
    auto& engine = AlarmEngine::instance();

    // 更新统计标签
    int count = engine.activeAlarmCount();
    m_count_label->setText(QString::fromUtf8("活跃报警: %1 条").arg(count));

    // 有活跃报警时显示红色，否则显示绿色
    m_count_label->setStyleSheet(
        count > 0
        ? "font-size: 16px; font-weight: bold; color: #E74C3C;"
        : "font-size: 16px; font-weight: bold; color: #2ECC71;");

    // 刷新表格，显示所有活动报警
    m_model->refresh(engine.getActiveAlarms());
}

// ------------------------------------------------------------
// onAcknowledge() — “确认报警”按钮槽函数
// ------------------------------------------------------------
// 获取当前选中的行，调用报警引擎确认该报警。
// 未选中任何行时弹出提示。
// ------------------------------------------------------------
void AlarmListWidget::onAcknowledge()
{
    // 获取当前选中的行
    auto index = m_view->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, QString::fromUtf8("提示"),
            QString::fromUtf8("请先选择一个报警"));
        return;
    }

    QString event_id = m_model->eventIdAt(index.row());
    if (AlarmEngine::instance().acknowledge(event_id.toStdString())) {
        refreshAlarms();
    }
}

// ------------------------------------------------------------
// onAcknowledgeClear() — “确认恢复”按钮槽函数
// ------------------------------------------------------------
// 获取当前选中的行，调用报警引擎确认该报警的恢复（彻底关闭）。
// 未选中任何行时弹出提示。
// ------------------------------------------------------------
void AlarmListWidget::onAcknowledgeClear()
{
    auto index = m_view->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, QString::fromUtf8("提示"),
            QString::fromUtf8("请先选择一个报警"));
        return;
    }

    QString event_id = m_model->eventIdAt(index.row());
    if (AlarmEngine::instance().acknowledgeClear(event_id.toStdString())) {
        refreshAlarms();
    }
}

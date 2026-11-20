// ============================================================
// MonitorWidget.cpp — 监控页面实现
//
// 整合工艺流程图、仪表盘、数据监视与当前配方信息。左右两部分
// 使用 QSplitter 分割，上下子区域同样使用 QSplitter，支持拖动
// 调整各部分比例。
// ============================================================

#include "MonitorWidget.h"
#include "ProcessView.h"
#include "DashboardWidget.h"
#include "DataTableWidget.h"
#include "alarm/AlarmTableModel.h"
#include "core/alarm/AlarmEngine.h"
#include "business/recipe/RecipeManager.h"
#include "rtdb/RealTimeDB.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QTableWidget>
#include <QTableView>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QStringList>
#include <QTimer>

MonitorWidget::MonitorWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    // 配方列表变化时刷新显示
    connect(&RecipeManager::instance(), &RecipeManager::recipesChanged,
        this, &MonitorWidget::refreshRecipeInfo);
    refreshRecipeInfo();

    // 定时刷新实时参数显示
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MonitorWidget::refreshParameters);
    timer->start(200);
    refreshParameters();

    // 定时刷新当前报警信息
    m_alarm_timer = new QTimer(this);
    connect(m_alarm_timer, &QTimer::timeout, this, &MonitorWidget::refreshAlarms);
    m_alarm_timer->start(500);
    refreshAlarms();
}

void MonitorWidget::setupUI()
{
    // 统一浅灰背景
    setStyleSheet(
        "MonitorWidget { background-color: #F0F0F0; color: #2C3E50; }"
        "MonitorWidget DashboardWidget { background-color: #F0F0F0; }"
        "MonitorWidget QSplitter::handle { background-color: #B0B0B0; }");

    auto* root_layout = new QHBoxLayout(this);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);

    // ---------------- 左半部分 ----------------
    // 参数显示 + 工艺流程图 + 下方仪表盘
    auto* left_splitter = new QSplitter(Qt::Vertical);

    // 实时参数显示（目标截图风格：名称 + 大数字 + 单位）
    auto* param_panel = new QWidget();
    param_panel->setStyleSheet(
        "QWidget { background-color: #FFFFFF; border-radius: 8px; }"
        "QLabel { color: #2ECC71; }");
    auto* param_layout = new QHBoxLayout(param_panel);
    param_layout->setContentsMargins(20, 8, 20, 8);

    auto make_param = [&](const QString& name, const QString& unit) {
        auto* w = new QWidget();
        auto* l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        auto* name_label = new QLabel(name);
        name_label->setStyleSheet("font-size: 13px; color: #7F8C8D;");
        auto* value_label = new QLabel("--");
        value_label->setStyleSheet("font-size: 22px; font-weight: bold; color: #2ECC71;");
        value_label->setProperty("unit", unit);
        l->addWidget(name_label);
        l->addWidget(value_label);
        param_layout->addWidget(w, 1);
        return value_label;
    };
    m_param_temp = make_param(QString::fromUtf8("釜内温度"), QString::fromUtf8("℃"));
    m_param_pressure = make_param(QString::fromUtf8("釜内压力"), QString::fromUtf8("MPa"));
    m_param_level = make_param(QString::fromUtf8("釜内液位"), QString::fromUtf8("%"));
    m_param_stage = make_param(QString::fromUtf8("工艺阶段"), QString());
    // 工艺阶段文字使用红色突出显示
    m_param_stage->setStyleSheet("font-size: 22px; font-weight: bold; color: #E74C3C;");

    m_process_view = new ProcessView();
    m_dashboard = new DashboardWidget();

    left_splitter->addWidget(param_panel);
    left_splitter->addWidget(m_process_view);
    left_splitter->addWidget(m_dashboard);
    left_splitter->setStretchFactor(0, 10);
    left_splitter->setStretchFactor(1, 60);   // 工艺流程图：仪表盘 = 3 : 2
    left_splitter->setStretchFactor(2, 40);

    // ---------------- 右半部分 ----------------
    // 上方数据监视 + 下方当前配方信息（固定宽度）
    auto* right_panel = new QWidget();
    auto* right_layout = new QVBoxLayout(right_panel);
    right_layout->setContentsMargins(0, 0, 0, 0);
    right_layout->setSpacing(4);

    m_data_table = new DataTableWidget();

    // 报警信息面板（固定高度，报警过多时内部垂直滚动）
    auto* alarm_panel = new QWidget();
    auto* alarm_layout = new QVBoxLayout(alarm_panel);
    alarm_layout->setContentsMargins(10, 10, 10, 10);
    alarm_layout->setSpacing(6);

    auto* alarm_title = new QLabel(QString::fromUtf8("当前报警信息"));
    alarm_title->setStyleSheet("font-size: 16px; font-weight: bold;");
    alarm_title->setAlignment(Qt::AlignCenter);
    alarm_layout->addWidget(alarm_title);

    m_alarm_model = new AlarmTableModel(this);
    m_alarm_view = new QTableView();
    m_alarm_view->setModel(m_alarm_model);
    m_alarm_view->setAlternatingRowColors(true);
    m_alarm_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alarm_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_alarm_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alarm_view->verticalHeader()->setVisible(false);
    m_alarm_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);   // 报警多时上下滑动
    m_alarm_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 固定宽度下不横向滚动
    // 前五列固定列宽，最后一列拉伸填满固定宽度
    m_alarm_view->setColumnWidth(0, 60);    // 时间
    m_alarm_view->setColumnWidth(1, 150);   // 报警描述
    m_alarm_view->setColumnWidth(2, 60);    // 位号
    m_alarm_view->setColumnWidth(3, 60);    // 触发值
    m_alarm_view->setColumnWidth(4, 55);    // 优先级
    m_alarm_view->horizontalHeader()->setStretchLastSection(true);
    alarm_layout->addWidget(m_alarm_view, 1);

    // 配方信息面板
    auto* recipe_panel = new QWidget();
    auto* recipe_layout = new QVBoxLayout(recipe_panel);
    recipe_layout->setContentsMargins(10, 10, 10, 10);
    recipe_layout->setSpacing(6);

    auto* recipe_title = new QLabel(QString::fromUtf8("当前配方信息"));
    recipe_title->setStyleSheet("font-size: 16px; font-weight: bold;");
    recipe_title->setAlignment(Qt::AlignCenter);
    recipe_layout->addWidget(recipe_title);

    m_recipe_summary = new QLabel();
    m_recipe_summary->setStyleSheet("font-size: 13px;");
    m_recipe_summary->setWordWrap(true);
    recipe_layout->addWidget(m_recipe_summary);

    m_recipe_table = new QTableWidget();
    m_recipe_table->setColumnCount(4);
    m_recipe_table->setHorizontalHeaderLabels({
        QString::fromUtf8("序号"),
        QString::fromUtf8("阶段"),
        QString::fromUtf8("设定值"),
        QString::fromUtf8("时长")
    });
    m_recipe_table->horizontalHeader()->setStretchLastSection(true);
    m_recipe_table->verticalHeader()->setVisible(false);
    m_recipe_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recipe_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_recipe_table->setAlternatingRowColors(true);
    recipe_layout->addWidget(m_recipe_table, 1);

    // 三个面板按 2 : 2 : 1 分配高度
    right_layout->addWidget(m_data_table, 2);
    right_layout->addWidget(alarm_panel, 2);
    right_layout->addWidget(recipe_panel, 1);
    right_panel->setFixedWidth(DataTableWidget::FIXED_WIDTH);

    // ---------------- 左右分割 ----------------
    root_layout->addWidget(left_splitter, 1);   // 左侧占剩余空间
    root_layout->addWidget(right_panel, 0);     // 右侧固定宽度
}

QString MonitorWidget::stageTargetText(const RecipeStage& stage)
{
    QStringList parts;
    if (stage.level_target)
        parts << QString::fromUtf8("液位 %1%").arg(*stage.level_target);
    if (stage.temp_target)
        parts << QString::fromUtf8("温度 %1℃").arg(*stage.temp_target);
    if (stage.agitator_speed)
        parts << QString::fromUtf8("搅拌 %1RPM").arg(*stage.agitator_speed);
    if (stage.valve_open)
        parts << QString::fromUtf8("阀门 %1").arg(*stage.valve_open);
    return parts.join(QString::fromUtf8("，"));
}

// 刷新实时参数显示（温度 / 压力 / 液位）
void MonitorWidget::refreshParameters()
{
    auto& rtdb = RealTimeDB::instance();
    TagDataPoint point;

    auto set_value = [](QLabel* label, const QString& text) {
        if (!label) return;
        label->setText(text);
    };

    if (rtdb.getTagValue("TE-101", point) && point.isValid()) {
        set_value(m_param_temp,
            QString::number(point.scaled_value, 'f', 2) + " " +
            m_param_temp->property("unit").toString());
    }
    else {
        set_value(m_param_temp, "--");
    }

    if (rtdb.getTagValue("PT-101", point) && point.isValid()) {
        set_value(m_param_pressure,
            QString::number(point.scaled_value, 'f', 2) + " " +
            m_param_pressure->property("unit").toString());
    }
    else {
        set_value(m_param_pressure, "--");
    }

    if (rtdb.getTagValue("LT-101", point) && point.isValid()) {
        set_value(m_param_level,
            QString::number(point.scaled_value, 'f', 1) + " " +
            m_param_level->property("unit").toString());
    }
    else {
        set_value(m_param_level, "--");
    }

    // 工艺阶段（来自工艺流程图当前阶段）
    if (m_process_view) {
        QString stage = m_process_view->currentStageText();
        set_value(m_param_stage,
            stage.isEmpty() ? QString::fromUtf8("待机") : stage);
    }
}

// 刷新当前报警信息显示
void MonitorWidget::refreshAlarms()
{
    if (!m_alarm_model) return;
    m_alarm_model->refresh(AlarmEngine::instance().getActiveAlarms());
}

void MonitorWidget::refreshRecipeInfo()
{
    auto recipes = RecipeManager::instance().getAllRecipes();

    if (recipes.empty()) {
        m_recipe_summary->setText(QString::fromUtf8("暂无配方"));
        m_recipe_table->setRowCount(0);
        return;
    }

    // 当前配方：默认取第一个配方
    const Recipe& recipe = recipes.front();

    m_recipe_summary->setText(
        QString::fromUtf8("配方编号: %1    产品: %2    版本: %3    作者: %4")
        .arg(QString::fromStdString(recipe.recipe_id))
        .arg(QString::fromStdString(recipe.product_name))
        .arg(QString::fromStdString(recipe.version))
        .arg(QString::fromStdString(recipe.author)));

    m_recipe_table->setRowCount(static_cast<int>(recipe.stages.size()));
    for (int i = 0; i < static_cast<int>(recipe.stages.size()); ++i) {
        const RecipeStage& stage = recipe.stages[i];

        m_recipe_table->setItem(i, 0,
            new QTableWidgetItem(QString::number(i + 1)));
        m_recipe_table->setItem(i, 1,
            new QTableWidgetItem(QString::fromStdString(stage.stage_name)));
        m_recipe_table->setItem(i, 2,
            new QTableWidgetItem(stageTargetText(stage)));
        m_recipe_table->setItem(i, 3,
            new QTableWidgetItem(stage.duration_sec > 0
                ? QString::fromUtf8("%1 s").arg(stage.duration_sec)
                : QString::fromUtf8("目标达成")));
    }
}

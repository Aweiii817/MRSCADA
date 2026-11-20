// ============================================================
// DashboardWidget.cpp — 仪表盘实现
// ============================================================
#include "DashboardWidget.h"
#include "GaugeWidget.h"
#include "LevelBar.h"
#include "StatusIndicator.h"
#include "rtdb/RealTimeDB.h"
#include "Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    // 创建并开启刷新定时器，连接超时信号
    m_refresh_timer = new QTimer(this);
    connect(m_refresh_timer, &QTimer::timeout,
        this, &DashboardWidget::refreshDisplay);
    m_refresh_timer->start(200);

    LOG_INFO("DashboardWidget created");
}

// 设置仪表盘的 UI 布局和控件
void DashboardWidget::setupUI()
{
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(16, 12, 16, 12);
    main_layout->setSpacing(12);

    auto* title = new QLabel(QString::fromUtf8("实时仪表盘"));
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: black;");
    title->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(title);

    // ---------- 反应釜区域 ----------
    auto* reactor_frame = new QFrame();
    reactor_frame->setStyleSheet(
        "QFrame { border: 2px dashed #888888; border-radius: 8px; }");
    auto* reactor_layout = new QVBoxLayout(reactor_frame);
    reactor_layout->setContentsMargins(10, 8, 10, 8);
    reactor_layout->setSpacing(8);

    auto* reactor_label = new QLabel(QString::fromUtf8("反应釜"));
    reactor_label->setStyleSheet("font-size: 15px; font-weight: bold; color: black;");
    reactor_label->setAlignment(Qt::AlignCenter);
    reactor_layout->addWidget(reactor_label);

    // 仪表区：温度表盘 / 压力表盘 / 液位柱
    auto* instruments_layout = new QHBoxLayout();
    instruments_layout->setSpacing(30);

    m_gauge_temp = new GaugeWidget();
    m_gauge_temp->setRange(0.0, 300.0);
    m_gauge_temp->setTitle(QString::fromUtf8("釜内温度"), QString::fromUtf8("TE-101"));
    m_gauge_temp->setUnit(QString::fromUtf8("°C"));
    m_gauge_temp->setHighLimit(200.0);

    m_gauge_pressure = new GaugeWidget();
    m_gauge_pressure->setRange(0.0, 2.5);
    m_gauge_pressure->setTitle(QString::fromUtf8("釜内压力"), QString::fromUtf8("PT-101"));
    m_gauge_pressure->setUnit(QString::fromUtf8("MPa"));
    m_gauge_pressure->setHighLimit(2.0);

    m_level_bar = new LevelBar();
    m_level_bar->setRange(0.0, 100.0);
    m_level_bar->setTitle(QString::fromUtf8("釜内液位"));
    m_level_bar->setUnit(QString::fromUtf8("%"));
    m_level_bar->setHighLimit(90.0);

    instruments_layout->addWidget(m_gauge_temp, 1);
    instruments_layout->addWidget(m_gauge_pressure, 1);
    instruments_layout->addWidget(m_level_bar, 1);
    reactor_layout->addLayout(instruments_layout, 1);

    // 阀门状态区：一排 LED 指示灯
    auto* valve_layout = new QHBoxLayout();
    valve_layout->setSpacing(20);

    m_led_valve_a = new StatusIndicator();
    m_led_valve_a->setLabel(QString::fromUtf8("进料A阀  HV-101"));

    m_led_valve_b = new StatusIndicator();
    m_led_valve_b->setLabel(QString::fromUtf8("进料B阀  HV-102"));

    m_led_valve_out = new StatusIndicator();
    m_led_valve_out->setLabel(QString::fromUtf8("出料阀  HV-103"));

    valve_layout->addStretch(1);
    valve_layout->addWidget(m_led_valve_a, 0);
    valve_layout->addStretch(1);
    valve_layout->addWidget(m_led_valve_b, 0);
    valve_layout->addStretch(1);
    valve_layout->addWidget(m_led_valve_out, 0);
    valve_layout->addStretch(1);

    reactor_layout->addLayout(valve_layout);
    main_layout->addWidget(reactor_frame, 1);

    // ---------- 原料罐区域（A / B 六个仪表同一行排列）----------
    auto* tank_frame = new QFrame();
    tank_frame->setStyleSheet(
        "QFrame { border: 2px dashed #888888; border-radius: 8px; }");
    auto* tank_layout = new QVBoxLayout(tank_frame);
    tank_layout->setContentsMargins(10, 8, 10, 8);
    tank_layout->setSpacing(8);

    auto* tank_label = new QLabel(QString::fromUtf8("原料罐 A / B"));
    tank_label->setStyleSheet("font-size: 15px; font-weight: bold; color: black;");
    tank_label->setAlignment(Qt::AlignCenter);
    tank_label->setFixedHeight(20);
    tank_layout->addWidget(tank_label);

    // 原料罐 A：液位（柱）/ 温度 / 压力（表盘）
    m_tank_a_level = new LevelBar();
    m_tank_a_level->setRange(0.0, 100.0);
    m_tank_a_level->setTitle(QString::fromUtf8("罐A液位"));
    m_tank_a_level->setUnit(QString::fromUtf8("%"));
    m_tank_a_level->setHighLimit(90.0);
    m_tank_a_level->setFixedSize(70, 190);

    m_tank_a_temp = new GaugeWidget();
    m_tank_a_temp->setRange(0.0, 100.0);
    m_tank_a_temp->setTitle(QString::fromUtf8("罐A温度"), QString::fromUtf8("TE-201"));
    m_tank_a_temp->setUnit(QString::fromUtf8("℃"));
    m_tank_a_temp->setFixedSize(180, 180);

    m_tank_a_pressure = new GaugeWidget();
    m_tank_a_pressure->setRange(0.0, 1.0);
    m_tank_a_pressure->setTitle(QString::fromUtf8("罐A压力"), QString::fromUtf8("PT-201"));
    m_tank_a_pressure->setUnit(QString::fromUtf8("MPa"));
    m_tank_a_pressure->setFixedSize(180, 180);

    // 原料罐 B：液位（柱）/ 温度 / 压力（表盘）
    m_tank_b_level = new LevelBar();
    m_tank_b_level->setRange(0.0, 100.0);
    m_tank_b_level->setTitle(QString::fromUtf8("罐B液位"));
    m_tank_b_level->setUnit(QString::fromUtf8("%"));
    m_tank_b_level->setHighLimit(90.0);
    m_tank_b_level->setFixedSize(70, 190);

    m_tank_b_temp = new GaugeWidget();
    m_tank_b_temp->setRange(0.0, 100.0);
    m_tank_b_temp->setTitle(QString::fromUtf8("罐B温度"), QString::fromUtf8("TE-202"));
    m_tank_b_temp->setUnit(QString::fromUtf8("℃"));
    m_tank_b_temp->setFixedSize(180, 180);

    m_tank_b_pressure = new GaugeWidget();
    m_tank_b_pressure->setRange(0.0, 1.0);
    m_tank_b_pressure->setTitle(QString::fromUtf8("罐B压力"), QString::fromUtf8("PT-202"));
    m_tank_b_pressure->setUnit(QString::fromUtf8("MPa"));
    m_tank_b_pressure->setFixedSize(180, 180);

    // 六个仪表放同一行：罐A（液位/温度/压力）+ 罐B（液位/温度/压力）
    auto* tank_instruments = new QHBoxLayout();
    tank_instruments->setSpacing(12);
    tank_instruments->addStretch(1);
    tank_instruments->addWidget(m_tank_a_level, 0);
    tank_instruments->addWidget(m_tank_a_temp, 0);
    tank_instruments->addWidget(m_tank_a_pressure, 0);
    tank_instruments->addSpacing(24);
    tank_instruments->addWidget(m_tank_b_level, 0);
    tank_instruments->addWidget(m_tank_b_temp, 0);
    tank_instruments->addWidget(m_tank_b_pressure, 0);
    tank_instruments->addStretch(1);
    tank_layout->addLayout(tank_instruments, 1);
    main_layout->addWidget(tank_frame, 1);
}

// 根据标签值更新阀门 LED 指示灯的状态
void DashboardWidget::updateValveLed(StatusIndicator* led,
    const std::string& tag_name)
{
    TagDataPoint point;
    auto& rtdb = RealTimeDB::instance();

    if (!rtdb.getTagValue(tag_name, point) || !point.isValid()) {
        led->setState(IndicatorState::ALARM);
        return;
    }

    led->setState(point.scaled_value > 0.5
        ? IndicatorState::ON : IndicatorState::OFF);
}

// 刷新仪表盘显示，从实时数据库获取最新数据并更新控件
void DashboardWidget::refreshDisplay()
{
    auto& rtdb = RealTimeDB::instance();
    TagDataPoint point;

    if (rtdb.getTagValue("TE-101", point)) {
        m_gauge_temp->setValid(point.isValid());
        m_gauge_temp->setValue(point.scaled_value);
    }

    if (rtdb.getTagValue("PT-101", point)) {
        m_gauge_pressure->setValid(point.isValid());
        m_gauge_pressure->setValue(point.scaled_value);
    }

    if (rtdb.getTagValue("LT-101", point)) {
        m_level_bar->setValid(point.isValid());
        m_level_bar->setValue(point.scaled_value);
    }

    // 原料罐 A 仪表盘（液位 / 温度 / 压力）
    if (rtdb.getTagValue("LT-201", point)) {
        m_tank_a_level->setValid(point.isValid());
        m_tank_a_level->setValue(point.scaled_value);
    }
    if (rtdb.getTagValue("TE-201", point)) {
        m_tank_a_temp->setValid(point.isValid());
        m_tank_a_temp->setValue(point.scaled_value);
    }
    if (rtdb.getTagValue("PT-201", point)) {
        m_tank_a_pressure->setValid(point.isValid());
        m_tank_a_pressure->setValue(point.scaled_value);
    }

    // 原料罐 B 仪表盘（液位 / 温度 / 压力）
    if (rtdb.getTagValue("LT-202", point)) {
        m_tank_b_level->setValid(point.isValid());
        m_tank_b_level->setValue(point.scaled_value);
    }
    if (rtdb.getTagValue("TE-202", point)) {
        m_tank_b_temp->setValid(point.isValid());
        m_tank_b_temp->setValue(point.scaled_value);
    }
    if (rtdb.getTagValue("PT-202", point)) {
        m_tank_b_pressure->setValid(point.isValid());
        m_tank_b_pressure->setValue(point.scaled_value);
    }

    updateValveLed(m_led_valve_a, "HV-101");
    updateValveLed(m_led_valve_b, "HV-102");
    updateValveLed(m_led_valve_out, "HV-103");
}

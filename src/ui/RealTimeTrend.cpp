// ============================================================
// RealTimeTrend.cpp — 实时趋势实现
// ============================================================
#include "RealTimeTrend.h"
#include "rtdb/RealTimeDB.h"
#include "tag/TagManager.h"
#include "Logger.h"

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QDateTime>

// 曲线颜色池初始化
const QList<QColor> RealTimeTrend::s_color_pool = {
    QColor("#3498DB"),   // 蓝
    QColor("#E74C3C"),   // 红
    QColor("#2ECC71")    // 绿
};

// 构造函数
RealTimeTrend::RealTimeTrend(QWidget* parent)
    : QWidget(parent)
{
    // 初始化UI和图表
    setupUI();
    setupChart();
    // 创建并开启定时器，连接定时器超时信号和刷新槽函数
    m_refresh_timer = new QTimer(this);
    connect(m_refresh_timer, &QTimer::timeout,
        this, &RealTimeTrend::refreshChart);
    m_refresh_timer->start(500);  
}

RealTimeTrend::~RealTimeTrend()
{
}

// 初始化UI
void RealTimeTrend::setupUI()
{
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(10, 10, 10, 10);
    main_layout->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(10);

    auto* label = new QLabel(QString::fromUtf8("测点:"));
    toolbar->addWidget(label);

    m_tag_combo = new QComboBox();
    // 从标签管理器获取全部标签导入标签下拉选择框
    const auto& tags = TagManager::instance().allTags();
    for (const auto& tag : tags) {
        if (tag->data_type == DataType::FLOAT ||
            tag->data_type == DataType::DOUBLE) {
            m_tag_combo->addItem(
                QString("%1 (%2)")
                .arg(QString::fromStdString(tag->tag_name))
                .arg(QString::fromStdString(tag->description)),
                QString::fromStdString(tag->tag_name));  
        }
    }
    toolbar->addWidget(m_tag_combo);

    m_add_btn = new QPushButton(QString::fromUtf8("添加曲线"));
    connect(m_add_btn, &QPushButton::clicked,
        this, &RealTimeTrend::onAddSeries);
    toolbar->addWidget(m_add_btn);

    m_clear_btn = new QPushButton(QString::fromUtf8("清空"));
    connect(m_clear_btn, &QPushButton::clicked,
        this, &RealTimeTrend::onClearSeries);
    toolbar->addWidget(m_clear_btn);

    toolbar->addStretch();
    main_layout->addLayout(toolbar);

    m_chart_view = new QChartView();
    // 设置抗锯齿
    m_chart_view->setRenderHint(QPainter::Antialiasing); 
    main_layout->addWidget(m_chart_view, 1);  
}

// 初始化图表
void RealTimeTrend::setupChart()
{
    m_chart = new QChart();
    m_chart->setTitle(QString::fromUtf8("实时趋势（最近 60 秒）"));
    m_chart->setAnimationOptions(QChart::NoAnimation);  
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    m_axis_x = new QDateTimeAxis();
    m_axis_x->setFormat("HH:mm:ss");       
    m_axis_x->setTickCount(7);             
    m_axis_x->setTitleText(QString::fromUtf8("时间"));
    m_chart->addAxis(m_axis_x, Qt::AlignBottom);

    m_axis_y = new QValueAxis();
    m_axis_y->setTitleText(QString::fromUtf8("数值"));
    m_axis_y->setLabelFormat("%.1f");      
    m_chart->addAxis(m_axis_y, Qt::AlignLeft);

    QDateTime now = QDateTime::currentDateTime();
    m_axis_x->setRange(now.addSecs(-WINDOW_SECONDS), now);

    m_axis_y->setRange(0, 100);

    m_chart_view->setChart(m_chart);
}

// 添加曲线按钮槽函数
void RealTimeTrend::onAddSeries()
{
    QString tag_name = m_tag_combo->currentData().toString();
    if (tag_name.isEmpty()) return;

    addSeriesForTag(tag_name);
}

// 添加曲线
void RealTimeTrend::addSeriesForTag(const QString& tag_name)
{
    if (m_series_map.contains(tag_name)) {
        LOG_DEBUG("RealTimeTrend: series for {} already exists",
            tag_name.toStdString());
        return;
    }

    if (m_series_map.size() >= s_color_pool.size()) {
        LOG_WARN("RealTimeTrend: max {} series allowed",
            s_color_pool.size());
        return;
    }

    auto* series = new QLineSeries();
    series->setName(tag_name);

    QColor color = s_color_pool.at(m_series_map.size());
    QPen pen(color, 2);
    series->setPen(pen);

    m_chart->addSeries(series);
    series->attachAxis(m_axis_x);
    series->attachAxis(m_axis_y);

    m_series_map.insert(tag_name, series);
    LOG_INFO("RealTimeTrend: added series for {}", tag_name.toStdString());
}

// 清空按钮槽函数
void RealTimeTrend::onClearSeries()
{
    m_chart->removeAllSeries();
    m_series_map.clear();
}

// 曲线刷新槽函数
void RealTimeTrend::refreshChart()
{
    if (m_series_map.isEmpty()) return;

    updateAllSeries();

    QDateTime now = QDateTime::currentDateTime();
    m_axis_x->setRange(now.addSecs(-WINDOW_SECONDS), now);

    autoScaleYAxis();
}

// 更新所有曲线
void RealTimeTrend::updateAllSeries()
{
    // 从实时数据库获取测点信息
    auto& rtdb = RealTimeDB::instance();

    for (auto it = m_series_map.begin(); it != m_series_map.end(); ++it) {
        const QString& tag_name = it.key();
        QLineSeries* series = it.value();

        auto history = rtdb.getTagHistory(tag_name.toStdString(), MAX_POINTS);
        if (history.empty()) continue;

        QVector<QPointF> points;
        points.reserve(static_cast<int>(history.size()));

        for (const auto& point : history) {
            points.append(QPointF(
                static_cast<double>(point.timestamp_ms),
                point.scaled_value));
        }

        series->replace(points);

        if (!history.empty()) {
            series->setName(QString("%1: %2")
                .arg(tag_name)
                .arg(history.back().scaled_value, 0, 'f', 1));
        }
    }
}

// 自动更新Y轴范围
void RealTimeTrend::autoScaleYAxis()
{
    double global_min = std::numeric_limits<double>::max();
    double global_max = std::numeric_limits<double>::lowest();

    // 获取测点的最大值和最小值
    for (auto* series : m_series_map) {
        const auto& points = series->points();
        for (const auto& p : points) {
            global_min = qMin(global_min, p.y());
            global_max = qMax(global_max, p.y());
        }
    }

    // 数据无效，使用默认值
    if (global_min > global_max) {
        m_axis_y->setRange(0, 100);
        return;
    }

    // 获取最大最小差范围的10%为边距
    double margin = (global_max - global_min) * 0.1;
    // 如果数据本身的范围小于 10（比如所有点几乎都集中在一个很小的区间内），
    // 直接扩展边距会导致范围仍然过小，图形看起来像一条直线。
    // 因此这里强制将 Y 轴范围设为以数据中点为中心、宽度为 10 的区间。
    if (global_max - global_min < 10.0) {
        double center = (global_max + global_min) / 2.0;
        global_min = center - 5.0;
        global_max = center + 5.0;
    }
    // 数据范围足够大：正常情况下，在最小值和最大值的基础上分别向外扩展 margin，
    else {
        global_min -= margin;
        global_max += margin;
    }

    m_axis_y->setRange(global_min, global_max);
}
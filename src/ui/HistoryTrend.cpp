// ============================================================
// HistoryTrend.cpp — 历史趋势实现
// ============================================================
//
// 实现 HistoryTrend 历史趋势查询界面。提供：
//   1. 选择测点与起止时间；
//   2. 从历史数据库（HistoryDB）查询数据并绘制折线趋势图；
//   3. 将查询结果导出为 CSV 文件。
// 图表使用 Qt Charts（QChart + QLineSeries + 时间/数值双坐标轴）。
// ============================================================

#include "HistoryTrend.h"
#include "history/HistoryDB.h"      // 历史数据库（查询/导出数据来源）
#include "tag/TagManager.h"         // 位号管理器（获取测点列表）

#include <QChart>                    // 图表对象
#include <QChartView>                // 图表视图（用于嵌入界面）
#include <QLineSeries>               // 折线数据系列
#include <QDateTimeAxis>             // 时间轴（X 轴）
#include <QValueAxis>                // 数值轴（Y 轴）

#include <QVBoxLayout>               // 垂直布局
#include <QHBoxLayout>               // 水平布局
#include <QFormLayout>               // 表单布局（标签+控件成对）
#include <QComboBox>                 // 下拉选择（测点）
#include <QDateTimeEdit>             // 日期时间编辑框
#include <QPushButton>               // 按钮
#include <QLabel>                    // 标签
#include <QFileDialog>               // 文件保存对话框
#include <QMessageBox>               // 消息提示框

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 仅调用 setupUI() 构建界面，所有控件在此完成创建与连接。
// ------------------------------------------------------------
HistoryTrend::HistoryTrend(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

HistoryTrend::~HistoryTrend() = default;

// ------------------------------------------------------------
// setupUI() — 构建界面
// ------------------------------------------------------------
// 整体布局（从上到下）：
//   1. 表单区：测点下拉框 + 开始时间 + 结束时间；
//   2. 按钮区：查询、导出 CSV；
//   3. 图表区：折线趋势图（X 轴时间、Y 轴数值）。
// ------------------------------------------------------------
void HistoryTrend::setupUI()
{
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(10, 10, 10, 10);

    // ---- 表单区 ----
    auto* form = new QFormLayout();
    form->setSpacing(8);

    // 测点下拉框：仅列出数值型（FLOAT/DOUBLE）位号
    m_tag_combo = new QComboBox();
    const auto& tags = TagManager::instance().allTags();
    for (const auto& tag : tags) {
        if (tag->data_type == DataType::FLOAT ||
            tag->data_type == DataType::DOUBLE) {
            m_tag_combo->addItem(
                QString::fromStdString(tag->tag_name));
        }
    }
    form->addRow(QString::fromUtf8("测点:"), m_tag_combo);

    // 开始时间：默认当前时间前 1 小时
    m_start_edit = new QDateTimeEdit(
        QDateTime::currentDateTime().addSecs(-3600));
    m_start_edit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_start_edit->setCalendarPopup(true);   // 弹出日历选择器
    form->addRow(QString::fromUtf8("开始时间:"), m_start_edit);

    // 结束时间：默认当前时间
    m_end_edit = new QDateTimeEdit(QDateTime::currentDateTime());
    m_end_edit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_end_edit->setCalendarPopup(true);
    form->addRow(QString::fromUtf8("结束时间:"), m_end_edit);

    main_layout->addLayout(form);

    // ---- 按钮区 ----
    auto* btn_layout = new QHBoxLayout();

    m_query_btn = new QPushButton(QString::fromUtf8("查询"));
    connect(m_query_btn, &QPushButton::clicked,
        this, &HistoryTrend::onQuery);
    btn_layout->addWidget(m_query_btn);

    m_export_btn = new QPushButton(QString::fromUtf8("导出 CSV"));
    connect(m_export_btn, &QPushButton::clicked,
        this, &HistoryTrend::onExportCsv);
    btn_layout->addWidget(m_export_btn);

    btn_layout->addStretch();   // 弹性空间，让按钮靠左
    main_layout->addLayout(btn_layout);

    // ---- 图表区 ----
    m_chart = new QChart();
    m_chart->setTitle(QString::fromUtf8("历史趋势"));

    // 创建折线系列并加入图表
    m_series = new QLineSeries();
    m_chart->addSeries(m_series);

    // X 轴：日期时间轴，显示格式为 "月-日 时:分"
    m_axis_x = new QDateTimeAxis();
    m_axis_x->setFormat("MM-dd HH:mm");
    m_axis_x->setTitleText(QString::fromUtf8("时间"));
    m_chart->addAxis(m_axis_x, Qt::AlignBottom);   // 底边
    m_series->attachAxis(m_axis_x);

    // Y 轴：数值轴，标签保留 1 位小数
    m_axis_y = new QValueAxis();
    m_axis_y->setLabelFormat("%.1f");
    m_chart->addAxis(m_axis_y, Qt::AlignLeft);     // 左边
    m_series->attachAxis(m_axis_y);

    // 图表视图：承载图表，开启抗锯齿渲染
    m_chart_view = new QChartView(m_chart);
    m_chart_view->setRenderHint(QPainter::Antialiasing);
    main_layout->addWidget(m_chart_view, 1);   // stretch=1，占据剩余空间
}

// ------------------------------------------------------------
// onQuery() — 查询按钮槽函数
// ------------------------------------------------------------
// 步骤：
//   1. 校验输入（测点非空、开始时间早于结束时间）；
//   2. 查询历史数据库；
//   3. 无数据则提示；
//   4. 有数据则更新折线数据、坐标轴范围与图表标题。
// ------------------------------------------------------------
void HistoryTrend::onQuery()
{
    QString tag_name = m_tag_combo->currentText();
    if (tag_name.isEmpty()) return;

    // 起止时间转换为毫秒时间戳
    qint64 start_ms = m_start_edit->dateTime().toMSecsSinceEpoch();
    qint64 end_ms = m_end_edit->dateTime().toMSecsSinceEpoch();

    // 校验：开始时间必须早于结束时间
    if (start_ms >= end_ms) {
        QMessageBox::warning(this, QString::fromUtf8("时间错误"),
            QString::fromUtf8("开始时间必须早于结束时间"));
        return;
    }

    // 查询历史数据
    auto records = HistoryDB::instance().queryHistory(
        tag_name.toStdString(), start_ms, end_ms);

    // 无数据提示
    if (records.empty()) {
        QMessageBox::information(this, QString::fromUtf8("无数据"),
            QString::fromUtf8("该时间段内没有历史数据。\n请确认历史存储已启动且时间范围内有数据。"));
        return;
    }

    // 将记录转换为 QPointF 列表（X=时间戳毫秒，Y=数值）
    QVector<QPointF> points;
    points.reserve(static_cast<int>(records.size()));
    for (const auto& rec : records) {
        points.append(QPointF(
            static_cast<double>(rec.timestamp_ms), rec.value));
    }
    m_series->replace(points);
    m_series->setName(tag_name);

    // 设置 X 轴范围为查询区间
    m_axis_x->setRange(m_start_edit->dateTime(), m_end_edit->dateTime());

    // 计算 Y 轴范围（数据最小/最大值，上下各留 10% 边距 + 1）
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();
    for (const auto& rec : records) {
        min_val = qMin(min_val, rec.value);
        max_val = qMax(max_val, rec.value);
    }
    double margin = (max_val - min_val) * 0.1 + 1.0;
    m_axis_y->setRange(min_val - margin, max_val + margin);

    // 更新标题，显示测点与记录数
    m_chart->setTitle(QString::fromUtf8("历史趋势 — %1 (%2 条记录)")
        .arg(tag_name)
        .arg(records.size()));
}

// ------------------------------------------------------------
// onExportCsv() — 导出 CSV 按钮槽函数
// ------------------------------------------------------------
// 查询当前选中的测点与时间区间（最多 10 万条），
// 通过文件对话框选择保存路径后调用 HistoryDB 导出。
// ------------------------------------------------------------
void HistoryTrend::onExportCsv()
{
    QString tag_name = m_tag_combo->currentText();
    if (tag_name.isEmpty()) return;

    qint64 start_ms = m_start_edit->dateTime().toMSecsSinceEpoch();
    qint64 end_ms = m_end_edit->dateTime().toMSecsSinceEpoch();

    // 查询（max_rows 设为 10 万，尽量导出完整数据）
    auto records = HistoryDB::instance().queryHistory(
        tag_name.toStdString(), start_ms, end_ms, 100000);

    if (records.empty()) {
        QMessageBox::information(this, QString::fromUtf8("无数据"),
            QString::fromUtf8("没有可导出的数据"));
        return;
    }

    // 弹出保存文件对话框，默认文件名 "<测点>_history.csv"
    QString file_path = QFileDialog::getSaveFileName(
        this, QString::fromUtf8("导出 CSV"),
        QString("%1_history.csv").arg(tag_name),
        QString::fromUtf8("CSV 文件 (*.csv)"));

    if (file_path.isEmpty()) return;   // 用户取消

    // 执行导出并提示结果
    if (HistoryDB::instance().exportToCsv(records, file_path)) {
        QMessageBox::information(this, QString::fromUtf8("导出成功"),
            QString::fromUtf8("已导出 %1 条记录到:\n%2")
            .arg(records.size()).arg(file_path));
    }
}

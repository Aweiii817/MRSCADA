// ============================================================
// RealTimeTrend.h — 实时趋势曲线
// ============================================================
#pragma once

#include <QWidget>
#include <QTimer>
#include <QMap>
#include <QString>

class QLineSeries;
class QChart;
class QChartView;
class QDateTimeAxis;
class QValueAxis;
class QComboBox;
class QPushButton;

class RealTimeTrend : public QWidget
{
    Q_OBJECT

public:
    explicit RealTimeTrend(QWidget* parent = nullptr);
    ~RealTimeTrend() override;

private slots:
    // 刷新曲线数据
    void refreshChart();
    // 添加曲线
    void onAddSeries();
	// 清除所有曲线
    void onClearSeries();

private:
	// 初始化UI
    void setupUI();
	// 初始化图表
    void setupChart();
    // 为测点添加曲线
    void addSeriesForTag(const QString& tag_name);
	// 更新所有曲线数据
    void updateAllSeries();
	// 自动调整Y轴范围
    void autoScaleYAxis();

	QChart* m_chart = nullptr;              // 图表对象
	QChartView* m_chart_view = nullptr;     // 图表视图对象
	QDateTimeAxis* m_axis_x = nullptr;      // X轴时间轴
	QValueAxis* m_axis_y = nullptr;         // Y轴数值轴

	QMap<QString, QLineSeries*> m_series_map;   // 位号到曲线的映射

	QComboBox* m_tag_combo;     // 测点选择下拉框        
    QPushButton* m_add_btn;     // 添加测点按钮
    QPushButton* m_clear_btn;   // 清空按钮       

    QTimer* m_refresh_timer;    // 曲线更新定时器

    static const QList<QColor> s_color_pool;    // 曲线颜色池

    static constexpr int WINDOW_SECONDS = 60;   // 窗口显示历史时间：最近60秒
    static constexpr int MAX_POINTS = 600;      // 曲线显示最大测点：最多600个
};
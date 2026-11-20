// ============================================================
// DataAnalysisWidget.h — 数据分析页面
//
// 将实时趋势与历史趋势整合到同一页面：
//   上方：实时趋势曲线；
//   下方：历史趋势查询。
// 上下两部分通过滚动区域承载，内容过高时自动显示垂直滚动条。
// ============================================================
#pragma once

#include <QWidget>

class RealTimeTrend;
class HistoryTrend;

class DataAnalysisWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataAnalysisWidget(QWidget* parent = nullptr);

private:
    void setupUI();
};

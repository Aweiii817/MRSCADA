// ============================================================
// DataAnalysisWidget.cpp — 数据分析页面实现
//
// 上方放置实时趋势，下方放置历史趋势。整体放入 QScrollArea，
// 当上下两部分内容高度超出可视区域时显示垂直滚动条。
// ============================================================

#include "DataAnalysisWidget.h"
#include "RealTimeTrend.h"
#include "HistoryTrend.h"

#include <QVBoxLayout>
#include <QScrollArea>

DataAnalysisWidget::DataAnalysisWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void DataAnalysisWidget::setupUI()
{
    // 滚动区域：宽度随视口自适应，垂直方向按需显示滚动条
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);

    // 内容容器：上下两部分，各占一半
    auto* content = new QWidget();
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(4, 4, 4, 4);
    content_layout->setSpacing(4);

    auto* realtime = new RealTimeTrend();
    auto* history = new HistoryTrend();

    // 设置最小高度，保证两部分可完整显示；高度不足时触发滚动条
    realtime->setMinimumHeight(400);
    history->setMinimumHeight(400);

    content_layout->addWidget(realtime, 1);
    content_layout->addWidget(history, 1);

    scroll->setWidget(content);

    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->addWidget(scroll);
}

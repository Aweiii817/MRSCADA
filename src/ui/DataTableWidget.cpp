// ============================================================
// DataTableWidget.cpp ��װ QTableView + TagTableModel + ���������
// ============================================================
#include "DataTableWidget.h"
#include "TagTableModel.h"
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>

DataTableWidget::DataTableWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // 标题
    auto* title = new QLabel(QString::fromUtf8("当前数据监控"));
    title->setStyleSheet("font-size: 16px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    m_model = new TagTableModel(this);

    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_view = new QTableView(this);
    m_view->setModel(m_proxy);      
    m_view->setSortingEnabled(true); 
    // 表格样式设置
    m_view->setAlternatingRowColors(true);  // 隔行变色   
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);    // 整行选择  
    m_view->verticalHeader()->setVisible(false);    // 隐藏行号   
    m_view->setShowGrid(true);

    // 固定列宽，关闭水平滚动条，垂直方向按需滚动
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_view->horizontalHeader()->setStretchLastSection(false);
    m_view->setColumnWidth(0, 90);   // 位号
    m_view->setColumnWidth(1, 110);  // 描述
    m_view->setColumnWidth(2, 100);  // 数值(含单位)
    m_view->setColumnWidth(3, 80);   // 质量
    m_view->setColumnWidth(4, 90);   // 更新时间
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    layout->addWidget(m_view, 1);

    // 固定整体宽度，保证各列刚好完整显示
    setFixedWidth(FIXED_WIDTH);

    // 初始化模型，加载标签 + 绑定信号
    m_model->initialize();
}
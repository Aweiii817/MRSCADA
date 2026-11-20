// ============================================================
// DataTableWidget.h — 数据表格页面
// DataTableWidget.h — 数据表格页面
//
// 封装 QTableView + TagTableModel + 排序代理。
// 作为"系统配置"页或者独立的数据监视页使用。
// ============================================================
#pragma once

#include <QWidget>

class QTableView;
class QSortFilterProxyModel;
class TagTableModel;

class DataTableWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataTableWidget(QWidget* parent = nullptr);

    // 固定宽度：列宽总和 + 垂直滚动条 + 左右边距，保证各列刚好完整显示
    static constexpr int FIXED_WIDTH = 510;

private:
    QTableView* m_view;             // 表格视图
    TagTableModel* m_model;         // 数据模型
    QSortFilterProxyModel* m_proxy; // 排序代理
};

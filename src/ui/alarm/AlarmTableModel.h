// ============================================================
// AlarmTableModel.h — 报警表格模型
// ============================================================
//
// AlarmTableModel 是报警表格的数据模型，继承自 QAbstractTableModel，
// 用于将报警事件列表（std::vector<AlarmEvent>）以表格形式展示在
// QTableView 中。
//
// 表格共 6 列：
//   0 时间     | 1 报警描述 | 2 位号 | 3 触发值 | 4 优先级 | 5 状态
//
// 除了显示文本（DisplayRole），模型还提供：
//   - 前景色（ForegroundRole）：优先级列按颜色、状态列按红/黄/绿高亮；
//   - 对齐方式（TextAlignmentRole）：触发值列右对齐。
// ============================================================

#pragma once

#include <QAbstractTableModel>   // 表格模型基类
#include <vector>                // std::vector

#include "core/alarm/AlarmEvent.h"   // 报警事件结构体（模型数据源）

class AlarmTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    // 构造函数
    explicit AlarmTableModel(QObject* parent = nullptr);

    // 刷新模型数据：用新的事件列表替换内部缓存并通知视图重绘
    void refresh(const std::vector<AlarmEvent>& events);

    // 获取指定行对应事件的事件 ID（用于确认/关闭操作）
    QString eventIdAt(int row) const;

    // ---- 重写 QAbstractTableModel 的虚函数 ----
    // 行数 = 事件数量
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    // 列数固定为 6
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    // 返回指定单元格、指定角色（显示/颜色/对齐）的数据
    QVariant data(const QModelIndex& index,
        int role = Qt::DisplayRole) const override;
    // 返回表头数据（列标题）
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

private:
    // 将报警状态枚举转换为中文文本
    static QString stateText(AlarmState s);

    // 将报警优先级枚举转换为中文文本
    static QString priorityText(AlarmPriority p);

    // 返回报警优先级对应的颜色
    static QColor priorityColor(AlarmPriority p);

    std::vector<AlarmEvent> m_events;   // 当前显示的事件列表缓存
};

// ============================================================
// AlarmTableModel.cpp — 报警表格模型实现
// ============================================================
//
// 实现 AlarmTableModel 报警表格模型，将报警事件以 6 列表格展示。
// 各列：时间 | 报警描述 | 位号 | 触发值 | 优先级 | 状态。
// 同时提供颜色（前景色）与对齐等显示角色的数据。
// ============================================================

#include "AlarmTableModel.h"

#include <QDateTime>   // 时间戳转字符串
#include <QColor>      // 颜色

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
AlarmTableModel::AlarmTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

// ------------------------------------------------------------
// refresh() — 刷新模型数据
// ------------------------------------------------------------
// 用新的事件列表替换内部缓存。beginResetModel/endResetModel
// 通知视图整个模型已重置，从而整体刷新表格显示。
// ------------------------------------------------------------
void AlarmTableModel::refresh(const std::vector<AlarmEvent>& events)
{
    beginResetModel();   // 通知视图：模型即将重置
    m_events = events;
    endResetModel();     // 通知视图：模型已重置，请重新读取
}

// ------------------------------------------------------------
// eventIdAt() — 获取指定行的事件 ID
// ------------------------------------------------------------
// 越界返回空字符串，否则返回该行事件的事件 ID。
// ------------------------------------------------------------
QString AlarmTableModel::eventIdAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_events.size()))
        return QString();
    return QString::fromStdString(m_events[row].event_id);
}

// ------------------------------------------------------------
// rowCount() — 返回行数
// ------------------------------------------------------------
// 父索引有效（表示嵌套子项）时返回 0；否则返回事件数量。
// ------------------------------------------------------------
int AlarmTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_events.size());
}

// ------------------------------------------------------------
// columnCount() — 返回列数
// ------------------------------------------------------------
// 固定 6 列。
// ------------------------------------------------------------
int AlarmTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return 6;
}

// ------------------------------------------------------------
// data() — 返回指定单元格的数据
// ------------------------------------------------------------
// 按角色（role）区分：
//   DisplayRole：显示文本（时间/描述/位号/触发值/优先级/状态）；
//   ForegroundRole：前景色（优先级列按颜色，状态列按状态红黄绿）；
//   TextAlignmentRole：对齐方式（触发值列右对齐）。
// ------------------------------------------------------------
QVariant AlarmTableModel::data(const QModelIndex& index, int role) const
{
    // 校验索引有效性（行越界则返回空）
    if (!index.isValid() ||
        index.row() >= static_cast<int>(m_events.size())) {
        return QVariant();
    }

    const AlarmEvent& ev = m_events[index.row()];

    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case 0:
            // 触发时间，格式化为 "时:分:秒"
            return QDateTime::fromMSecsSinceEpoch(
                static_cast<qint64>(ev.trigger_time_ms))
                .toString("HH:mm:ss");
        case 1: return QString::fromStdString(ev.message);          // 报警描述
        case 2: return QString::fromStdString(ev.tag_name);         // 位号
        case 3: return QString::number(ev.trigger_value, 'f', 1);   // 触发值（保留 1 位小数）
        case 4: return priorityText(ev.priority);                   // 优先级文本
        case 5: return stateText(ev.state);                         // 状态文本
        }
        break;
    }

    case Qt::ForegroundRole:
        // 优先级列按颜色显示，状态列“未确认”用红色高亮
        if (index.column() == 4) {
            return priorityColor(ev.priority);
        }
        if (index.column() == 5) {
            if (ev.state == AlarmState::ACTIVE)
                return QColor("#E74C3C");   // 未确认 = 红
            if (ev.state == AlarmState::CLEARED_UNACK)
                return QColor("#F39C12");   // 恢复待确认 = 黄
            return QColor("#2ECC71");        // 已确认/已关闭 = 绿
        }
        return QColor("#ECF0F1");            // 其余列默认浅灰白

    case Qt::TextAlignmentRole:
        if (index.column() == 3)
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter); // 触发值右对齐
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);      // 其余左对齐
    }

    return QVariant();
}

// ------------------------------------------------------------
// headerData() — 返回表头数据
// ------------------------------------------------------------
// 仅处理水平方向的 DisplayRole，返回 6 个列标题。
// ------------------------------------------------------------
QVariant AlarmTableModel::headerData(int section,
    Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return QString::fromUtf8("时间");
        case 1: return QString::fromUtf8("报警描述");
        case 2: return QString::fromUtf8("位号");
        case 3: return QString::fromUtf8("触发值");
        case 4: return QString::fromUtf8("优先级");
        case 5: return QString::fromUtf8("状态");
        }
    }
    return QVariant();
}

// ------------------------------------------------------------
// stateText() — 状态枚举转中文
// ------------------------------------------------------------
QString AlarmTableModel::stateText(AlarmState s)
{
    switch (s) {
    case AlarmState::ACTIVE:        return QString::fromUtf8("未确认");
    case AlarmState::ACKNOWLEDGED:  return QString::fromUtf8("已确认");
    case AlarmState::CLEARED_UNACK: return QString::fromUtf8("恢复待确认");
    case AlarmState::INACTIVE:      return QString::fromUtf8("已关闭");
    default:                        return QString::fromUtf8("未知");
    }
}

// ------------------------------------------------------------
// priorityText() — 优先级枚举转中文
// ------------------------------------------------------------
QString AlarmTableModel::priorityText(AlarmPriority p)
{
    switch (p) {
    case AlarmPriority::CRITICAL: return QString::fromUtf8("危急");
    case AlarmPriority::HIGH:     return QString::fromUtf8("高");
    case AlarmPriority::MEDIUM:   return QString::fromUtf8("中");
    case AlarmPriority::LOW:      return QString::fromUtf8("低");
    default:                      return QString::fromUtf8("?");
    }
}

// ------------------------------------------------------------
// priorityColor() — 优先级对应颜色
// ------------------------------------------------------------
// 危急=深红、高=红、中=橙、低=绿。
// ------------------------------------------------------------
QColor AlarmTableModel::priorityColor(AlarmPriority p)
{
    switch (p) {
    case AlarmPriority::CRITICAL: return QColor("#C0392B");  // 深红
    case AlarmPriority::HIGH:     return QColor("#E74C3C");  // 红
    case AlarmPriority::MEDIUM:   return QColor("#F39C12");  // 橙
    case AlarmPriority::LOW:      return QColor("#2ECC71");  // 绿
    default:                      return QColor("#7F8C8D");  // 灰
    }
}

// ============================================================
// TagTableModel.cpp — 表格模型实现
// ============================================================
#include "TagTableModel.h"
#include "tag/TagManager.h"
#include "rtdb/RealTimeDB.h"
#include <QDateTime>
#include <QColor>

TagTableModel::TagTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

// 初始化：从 TagManager 加载所有标签作为行
void TagTableModel::initialize()
{
    // 告诉 View "整个模型的数据都要换了"，View 会完全重绘
    beginResetModel();

    m_rows.clear();
    m_row_index.clear();
    // 从 TagManager 加载所有标签
    const auto& tags = TagManager::instance().allTags();
    int row = 0;
    for (const auto& tag : tags) {
        m_rows.push_back(tag->tag_name);
        m_row_index[tag->tag_name] = row;
        ++row;
    }

    endResetModel();
    // 连接RealTimeDB 的测点值变化信号至模型值更新处理槽函数
    connect(&RealTimeDB::instance(), &RealTimeDB::tagValueChanged,
        this, &TagTableModel::onTagValueChanged);
}

int TagTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_rows.size());
}

int TagTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return 5;  // 5 列：位号/描述/数值(含单位)/质量/时间
}

QVariant TagTableModel::data(const QModelIndex& index, int role) const
{
    // 防御：索引越界直接返回空
    if (!index.isValid() ||
        index.row() < 0 ||
        index.row() >= static_cast<int>(m_rows.size())) {
        return QVariant();
    }
    // 找到这一行对应的位号
    const std::string& tag_name = m_rows[index.row()];

    // 从 RealTimeDB 取最新值
    TagDataPoint point;
    bool has_data = RealTimeDB::instance().getTagValue(tag_name, point);

    // 按角色返回不同内容
    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case 0: return QString::fromStdString(tag_name);
        case 1: {
            const auto* def = TagManager::instance().findTag(tag_name);
            return def ? QString::fromStdString(def->description) : "";
        }
        case 2: {
            if (!has_data) return "---";
            // 数值与单位合并显示
            QString value;
            if (point.data_type == DataType::BOOL)
                value = (point.scaled_value > 0.5) ? "ON" : "OFF";
            else
                value = QString::number(point.scaled_value, 'f', 1);

            const auto* def = TagManager::instance().findTag(tag_name);
            const QString unit = def ? QString::fromStdString(def->unit) : "";
            return unit.isEmpty() ? value : value + " " + unit;
        }
        case 3:
            return has_data ? qualityText(point.quality) : QString::fromUtf8("无数据");
        case 4: {
            if (!has_data) return "";
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(
                static_cast<qint64>(point.timestamp_ms));
            return dt.toString("HH:mm:ss");
        }
        }
        break;
    }

    case Qt::ForegroundRole: {
        if (index.column() == 2 || index.column() == 3) {
            if (!has_data) return QColor("#7F8C8D");
            return qualityColor(point.quality);
        }
        return QColor(Qt::black);  
    }

    case Qt::TextAlignmentRole:
        return static_cast<int>(Qt::AlignCenter);

    case Qt::ToolTipRole:
        if (has_data) {
            return QString::fromUtf8("位号: %1\n原始值: %2\n时间: %3")
                .arg(QString::fromStdString(tag_name))
                .arg(point.raw_value)
                .arg(QDateTime::fromMSecsSinceEpoch(
                    static_cast<qint64>(point.timestamp_ms))
                    .toString("yyyy-MM-dd HH:mm:ss.zzz"));
        }
        break;
    }

    return QVariant();
}

QVariant TagTableModel::headerData(int section,
    Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return QString::fromUtf8("位号");
        case 1: return QString::fromUtf8("描述");
        case 2: return QString::fromUtf8("数值");
        case 3: return QString::fromUtf8("质量");
        case 4: return QString::fromUtf8("更新时间");
        }
    }
    return QVariant();
}

void TagTableModel::onTagValueChanged(const QString& tag_name)
{
    auto it = m_row_index.find(tag_name.toStdString());
    if (it == m_row_index.end()) return;

    int row = it->second;

    QModelIndex top_left = createIndex(row, 0);
    QModelIndex bottom_right = createIndex(row, columnCount() - 1);
    emit dataChanged(top_left, bottom_right);
}

QString TagTableModel::qualityText(Quality q)
{
    switch (q) {
    case Quality::GOOD:               return QString::fromUtf8("正常");
    case Quality::UNCERTAIN:          return QString::fromUtf8("可疑");
    case Quality::BAD:                return QString::fromUtf8("故障");
    case Quality::GOOD_LOCAL_OVERRIDE: return QString::fromUtf8("手动");
    default:                          return QString::fromUtf8("未知");
    }
}

QColor TagTableModel::qualityColor(Quality q)
{
    switch (q) {
    case Quality::GOOD:               return QColor("#2ECC71");  
    case Quality::UNCERTAIN:          return QColor("#F39C12");  
    case Quality::BAD:                return QColor("#E74C3C");  
    case Quality::GOOD_LOCAL_OVERRIDE: return QColor("#3498DB"); 
    default:                          return QColor("#7F8C8D");
    }
}
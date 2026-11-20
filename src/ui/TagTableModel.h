// ============================================================
// TagTableModel.h — 标签表格模型
// 把 RealTimeDB 中的所有测点包装成表格模型。
// 列定义：
//   0: 位号      1: 描述    2: 数值(含单位)
//   3: 质量      4: 更新时间
// ============================================================
#pragma once

#include <QAbstractTableModel>
#include <vector>
#include <unordered_map>
#include <string>
#include "tag/TagDataPoint.h"

class QColor;

class TagTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TagTableModel(QObject* parent = nullptr);
    // 初始化：从 TagManager 加载所有标签作为行
    void initialize();
  
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

private slots:
    // RealTimeDB 的值变更通知槽函数
    void onTagValueChanged(const QString& tag_name);

private:
    // 把质量枚举转化为显示文字
    static QString qualityText(Quality q);
    // 根据质量返回文字颜色
    static QColor qualityColor(Quality q);

    std::vector<std::string> m_rows;    // 按行保存标签位号
    std::unordered_map<std::string, int> m_row_index;   // 位号映射行号
};

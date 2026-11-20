// ============================================================
// ValveItem.h — 阀门图元
// ============================================================
#pragma once

#include <QGraphicsItem>
#include <QString>

class ValveItem : public QGraphicsItem
{
public:
    explicit ValveItem(const QString& tag_name);
    // 
    QRectF boundingRect() const override;
    void paint(QPainter* painter,
        const QStyleOptionGraphicsItem* option,
        QWidget* widget) override;
    // 设置阀门开关状态
    void setOpen(bool open);
    // 设置中文标签（显示在阀门下方，用于说明阀门的作用）
    void setLabel(const QString& text);
    // 获取绑定的位号  
    QString tagName() const;

private:
    QString m_tag_name;   // 绑定的测点位号
    QString m_label;      // 显示文字
    bool m_open = false;  // 当前开关状态

    // 阀门图元尺寸
    static constexpr qreal VALVE_WIDTH = 30;
    static constexpr qreal VALVE_HEIGHT = 20;
};
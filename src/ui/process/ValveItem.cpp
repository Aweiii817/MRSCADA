// ============================================================
// ValveItem.cpp — 阀门图元实现
// ============================================================
#include "ValveItem.h"
#include <QPainter>

ValveItem::ValveItem(const QString& tag_name)
    : m_tag_name(tag_name)
{
    // 启用缓存模式：图元内容不变时，Qt 会把 paint 结果
    // 缓存成位图，下次直接贴图，大幅提升性能
    setCacheMode(DeviceCoordinateCache);

    // 允许接收鼠标悬浮事件（显示提示用）
    setAcceptHoverEvents(true);
    setToolTip(QString::fromUtf8("阀门 %1").arg(tag_name));
}

QRectF ValveItem::boundingRect() const
{
    return QRectF(-VALVE_WIDTH / 2 - 5,
        -VALVE_HEIGHT / 2 - 5,
        VALVE_WIDTH + 10,
        VALVE_HEIGHT + 44);  
}

// 
void ValveItem::paint(QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // 开启抗锯齿
    painter->setRenderHint(QPainter::Antialiasing);
    // 根据状态选择颜色
    QColor color = m_open ? QColor("#2ECC71")   
        : QColor("#E74C3C");  

    painter->setPen(QPen(color.darker(130), 2));
    painter->setBrush(color);

    qreal w = VALVE_WIDTH / 2;
    qreal h = VALVE_HEIGHT / 2;

    QPolygonF left_triangle;
    left_triangle << QPointF(-w, -h)
        << QPointF(-w, h)
        << QPointF(0, 0);
    painter->drawPolygon(left_triangle);

    QPolygonF right_triangle;
    right_triangle << QPointF(w, -h)
        << QPointF(w, h)
        << QPointF(0, 0);
    painter->drawPolygon(right_triangle);
    // 画阀门位号文字
    painter->setPen(QColor(Qt::black));
    QFont font = painter->font();
    font.setPointSize(6);
    painter->setFont(font);
    painter->drawText(QRectF(-30, h + 2, 60, 14),
        Qt::AlignCenter, m_tag_name);

    // 画阀门中文标签（说明阀门的作用）
    if (!m_label.isEmpty()) {
        painter->drawText(QRectF(-30, h + 16, 60, 14),
            Qt::AlignCenter, m_label);
    }
}

// 设置阀门状态
void ValveItem::setOpen(bool open)
{
    if (m_open != open) {
        m_open = open;
        update();   
    }
}

// 设置阀门中文标签
void ValveItem::setLabel(const QString& text)
{
    m_label = text;
    update();
}

// 获取绑定的测点位号
QString ValveItem::tagName() const
{
    return m_tag_name;
}
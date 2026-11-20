// ============================================================
// TankItem.cpp — 原料罐图元实现
// ============================================================
#include "TankItem.h"
#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QToolTip>

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
TankItem::TankItem()
{
    // 位图缓存，减少重复绘制开销
    setCacheMode(DeviceCoordinateCache);
    // 允许接收悬浮事件
    setAcceptHoverEvents(true);
    setToolTip(QString::fromUtf8("原料罐"));
}

// ------------------------------------------------------------
// boundingRect() — 外接矩形（中心原点，四周留出描边余量）
// ------------------------------------------------------------
QRectF TankItem::boundingRect() const
{
    return QRectF(-BODY_WIDTH / 2 - 10,
        -BODY_HEIGHT / 2 - 10,
        BODY_WIDTH + 20,
        BODY_HEIGHT + 20);
}

// ------------------------------------------------------------
// paint() — 绘制图元
// ------------------------------------------------------------
void TankItem::paint(QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    qreal w = BODY_WIDTH;
    qreal h = BODY_HEIGHT;
    qreal x = -w / 2;
    qreal y = -h / 2;

    // 罐体：蓝色描边 + 浅蓝填充
    painter->setPen(QPen(QColor("#4A90D9"), 2));
    painter->setBrush(QColor("#D6EAF8"));
    painter->drawRoundedRect(QRectF(x, y, w, h), 10, 10);

    // 液位填充（罐体内部，从底部向上）
    painter->save();
    QRectF inner_rect(x + 4, y + 4, w - 8, h - 8);
    painter->setClipRect(inner_rect);

    double ratio = qBound(0.0, m_level / 100.0, 1.0);
    qreal fill_h = inner_rect.height() * ratio;
    if (fill_h > 0) {
        QLinearGradient grad(0, inner_rect.bottom() - fill_h,
            0, inner_rect.bottom());
        grad.setColorAt(0.0, QColor("#2ECC71").lighter(130));
        grad.setColorAt(1.0, QColor("#2ECC71").darker(120));
        painter->setPen(Qt::NoPen);
        painter->setBrush(grad);
        painter->drawRect(QRectF(inner_rect.left(),
            inner_rect.bottom() - fill_h,
            inner_rect.width(),
            fill_h));
    }
    painter->restore();

    // 罐内显示温度 / 压力 / 液位（红色文字，在液位填充之上）
    painter->setPen(QColor("#E74C3C"));
    QFont font = painter->font();
    font.setPointSize(6);
    font.setBold(true);
    painter->setFont(font);

    painter->drawText(QRectF(x, y + 5, w, 12), Qt::AlignCenter,
        QString::number(m_temperature, 'f', 1) + QString::fromUtf8("℃"));
    painter->drawText(QRectF(x, y + 20, w, 12), Qt::AlignCenter,
        QString::number(m_pressure, 'f', 2) + QString::fromUtf8("MPa"));
    painter->drawText(QRectF(x, y + h - 18, w, 12), Qt::AlignCenter,
        QString::number(m_level, 'f', 0) + QString::fromUtf8("%"));
}

// ------------------------------------------------------------
// setLevel() — 设置液位（百分比 0~100）
// ------------------------------------------------------------
void TankItem::setLevel(double percent)
{
    double new_level = qBound(0.0, percent, 100.0);
    if (qAbs(new_level - m_level) > 0.5) {
        m_level = new_level;
        update();
    }
}

// ------------------------------------------------------------
// setTemperature() — 设置温度（℃）
// ------------------------------------------------------------
void TankItem::setTemperature(double celsius)
{
    if (qAbs(celsius - m_temperature) > 0.3) {
        m_temperature = celsius;
        update();
    }
}

// ------------------------------------------------------------
// setPressure() — 设置压力（MPa）
// ------------------------------------------------------------
void TankItem::setPressure(double mpa)
{
    if (qAbs(mpa - m_pressure) > 0.01) {
        m_pressure = mpa;
        update();
    }
}

// ------------------------------------------------------------
// hoverEnterEvent() — 悬浮提示
// ------------------------------------------------------------
void TankItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    setToolTip(QString::fromUtf8("原料罐\n"
        "温度: %1 ℃\n"
        "压力: %2 MPa\n"
        "液位: %3 %")
        .arg(m_temperature, 0, 'f', 1)
        .arg(m_pressure, 0, 'f', 2)
        .arg(m_level, 0, 'f', 1));

    QGraphicsItem::hoverEnterEvent(event);
}

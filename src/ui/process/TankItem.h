// ============================================================
// TankItem.h — 原料罐图元
// ============================================================
#pragma once

#include <QGraphicsItem>
#include <QString>

// 原料罐图元：在工艺流程图（ProcessView）中绘制一个原料储罐，
// 支持实时显示液位（底部填充）、温度与压力数值。
class TankItem : public QGraphicsItem
{
public:
    explicit TankItem();

    QRectF boundingRect() const override;
    void paint(QPainter* painter,
        const QStyleOptionGraphicsItem* option,
        QWidget* widget) override;

    // 设置液位（百分比 0~100）
    void setLevel(double percent);
    // 设置温度（℃）
    void setTemperature(double celsius);
    // 设置压力（MPa）
    void setPressure(double mpa);

protected:
    // 悬浮提示
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

private:
    double m_level = 100.0;       // 液位（%）
    double m_temperature = 25.0;  // 温度（℃）
    double m_pressure = 0.1;      // 压力（MPa）

    // 罐体尺寸（宽 x 高）
    static constexpr qreal BODY_WIDTH = 60;
    static constexpr qreal BODY_HEIGHT = 80;
};

// ============================================================
// PumpItem.h — 泵图元
// ============================================================
//
// PumpItem 表示工艺流程图中一台输送泵，继承自 QGraphicsItem。
// 外观为圆形泵体 + 内部旋转叶片，通过叶片旋转动画表示泵正在
// 运行；停止时叶片静止、颜色变灰。泵体下方显示位号（如 P-101）。
// ============================================================

#pragma once

#include <QGraphicsItem>   // QGraphicsItem 基类
#include <QString>         // QString 字符串

class PumpItem : public QGraphicsItem
{
public:
    // 构造函数：传入泵的位号（如 "P-101"）
    explicit PumpItem(const QString& tag_name);

    // 返回图元外接矩形（供场景判断重绘范围与命中检测）
    QRectF boundingRect() const override;

    // 绘制函数：绘制泵体、叶片与位号
    void paint(QPainter* painter,
        const QStyleOptionGraphicsItem* option,
        QWidget* widget) override;

    // 设置泵是否运行（运行 = 绿色并旋转叶片，停止 = 灰色静止）
    void setRunning(bool running);

    // 设置中文标签（显示在泵体下方，用于说明泵的作用）
    void setLabel(const QString& text);

    // 推进叶片旋转动画一帧（由外部定时器周期调用）
    void advanceAnimation();

private:
    QString m_tag_name;         // 泵位号（如 "P-101"）
    QString m_label;            // 中文标签（如 "进料泵A"）
    bool m_running = false;     // 是否处于运行状态
    qreal m_rotation = 0.0;     // 叶片当前旋转角度（度）

    static constexpr qreal RADIUS = 16;   // 泵体半径
};

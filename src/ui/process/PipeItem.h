// ============================================================
// PipeItem.h — 管道图元
// ============================================================
//
// PipeItem 表示工艺流程图中的一段管道，继承自 QGraphicsItem，
// 用于在设备/阀门之间绘制连接管线。它支持两种显示状态：
//   - 静止状态：黄色虚线；
//   - 流动状态：绿色虚线，并通过虚线偏移动画模拟物料沿管道流动。
//
// 说明：
//   管道由多个顶点组成的折线（m_points）定义，因此可以绘制
//   “平行 + 直角弯折”的管线路径。顶点数 >= 2 时按折线绘制，
//   相邻顶点之间用直线段连接。
// ============================================================

#pragma once

#include <QGraphicsItem>   // QGraphicsItem 基类
#include <QPointF>         // QPointF 二维坐标点
#include <QVector>         // QVector 顶点容器

class PipeItem : public QGraphicsItem
{
public:
    // 构造函数：传入折线顶点序列（至少两个点）
    explicit PipeItem(const QVector<QPointF>& points);

    // 构造函数：传入起点与终点坐标（等价于两顶点的折线）
    explicit PipeItem(const QPointF& from, const QPointF& to);

    // 返回图元外接矩形（供场景判断重绘范围与命中检测）
    QRectF boundingRect() const override;

    // 绘制函数：绘制管道折线
    void paint(QPainter* painter,
        const QStyleOptionGraphicsItem* option,
        QWidget* widget) override;

    // 设置管道是否处于流动状态（true 时用绿色虚线动画表示流动）
    void setFlowing(bool flowing);

    // 推进流动动画一帧（由外部定时器周期调用）
    void advanceAnimation();

private:
    QVector<QPointF> m_points;      // 折线顶点序列
    bool m_flowing = false;         // 是否处于流动状态
    qreal m_dash_offset = 0.0;      // 虚线偏移量（用于流动动画）
};

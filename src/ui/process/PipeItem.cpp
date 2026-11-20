// ============================================================
// PipeItem.cpp — 管道图元实现
// ============================================================
//
// 实现 PipeItem 管道图元：由多个顶点组成的折线管道，
// 可绘制“平行 + 直角弯折”的管线路径。
//   静止时为黄色虚线；流动时切换为绿色虚线，并通过不断改变虚线
// 偏移量（m_dash_offset）让虚线看起来像在“往前跑”，从而模拟
// 物料在管道中流动的动画效果。
// ============================================================

#include "PipeItem.h"      // 管道图元类声明
#include <QPainter>        // Qt 2D 绘制引擎

// ------------------------------------------------------------
// 构造函数（折线顶点）
// ------------------------------------------------------------
// 保存折线顶点序列，并开启位图缓存以提升绘制性能。
// 顶点数不足 2 个时补足到 2 个，避免绘制异常。
// ------------------------------------------------------------
PipeItem::PipeItem(const QVector<QPointF>& points)
    : m_points(points)
{
    if (m_points.size() < 2) {
        // 顶点不足时用原点补齐，保证至少能绘制一条线段
        m_points.append(QPointF(0, 0));
        m_points.append(QPointF(0, 0));
    }

    // 内容不变时，Qt 将绘制结果缓存为位图，后续直接贴图，减少重复绘制开销
    setCacheMode(DeviceCoordinateCache);
}

// ------------------------------------------------------------
// 构造函数（两点直线）
// ------------------------------------------------------------
// 兼容旧用法：两点折线即直线。
// ------------------------------------------------------------
PipeItem::PipeItem(const QPointF& from, const QPointF& to)
    : PipeItem(QVector<QPointF>{from, to})
{
}

// ------------------------------------------------------------
// boundingRect() — 返回图元外接矩形
// ------------------------------------------------------------
// 遍历所有顶点求包围盒，再向四周各外扩 10 像素，以容纳
// 管线的描边宽度（4 像素）与抗锯齿边缘，避免边缘被裁剪。
// ------------------------------------------------------------
QRectF PipeItem::boundingRect() const
{
    if (m_points.isEmpty()) {
        return QRectF();
    }

    // 用第一个顶点初始化包围盒
    qreal min_x = m_points.first().x();
    qreal min_y = m_points.first().y();
    qreal max_x = min_x;
    qreal max_y = min_y;

    // 遍历其余顶点，扩展包围盒
    for (int i = 1; i < m_points.size(); ++i) {
        const QPointF& p = m_points.at(i);
        min_x = qMin(min_x, p.x());
        min_y = qMin(min_y, p.y());
        max_x = qMax(max_x, p.x());
        max_y = qMax(max_y, p.y());
    }

    return QRectF(QPointF(min_x, min_y), QPointF(max_x, max_y))
        .adjusted(-10, -10, 10, 10); // 四周各外扩 10 像素
}

// ------------------------------------------------------------
// paint() — 绘制管道折线
// ------------------------------------------------------------
// 根据流动状态选择画笔：
//   - 流动状态：绿色虚线，并设置虚线偏移量形成流动动画；
//   - 静止状态：黄色虚线（固定偏移）。
// 最后绘制整条折线。
// ------------------------------------------------------------
void PipeItem::paint(QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget)
{
    Q_UNUSED(option);   // 本图元不使用样式选项
    Q_UNUSED(widget);   // 本图元不使用目标 widget

    QPen pen;
    pen.setWidth(4);                    // 4 像素宽
    pen.setStyle(Qt::DashLine);         // 虚线样式
    pen.setCapStyle(Qt::RoundCap);      // 圆角端点
    pen.setJoinStyle(Qt::RoundJoin);    // 折线拐角圆滑

    if (m_flowing) {
        pen.setColor(QColor("#2ECC71")); // 流动：绿色虚线
        pen.setDashOffset(m_dash_offset); // 偏移量循环变化，模拟流动
    }
    else {
        pen.setColor(QColor("#F1C40F")); // 静止：黄色虚线
    }

    painter->setPen(pen);

    // 绘制折线（顶点数 >= 2）
    painter->drawPolyline(m_points.constData(), m_points.size());
}

// ------------------------------------------------------------
// setFlowing() — 设置流动状态
// ------------------------------------------------------------
// 仅当状态发生改变时才更新并触发重绘，避免无谓刷新。
// ------------------------------------------------------------
void PipeItem::setFlowing(bool flowing)
{
    if (m_flowing != flowing) {
        m_flowing = flowing;
        update();   // 请求重绘
    }
}

// ------------------------------------------------------------
// advanceAnimation() — 推进流动动画一帧
// ------------------------------------------------------------
// 仅在流动状态下生效。每调用一次，虚线偏移量加 1，使虚线
// 沿管道方向移动；当偏移超过一个虚线周期（8 像素）时归零，
// 形成循环滚动的流动动画。由外部定时器周期调用本函数驱动。
// ------------------------------------------------------------
void PipeItem::advanceAnimation()
{
    if (!m_flowing) return;   // 静止状态不产生动画

    m_dash_offset += 1.0;     // 虚线向前偏移一帧

    if (m_dash_offset > 8.0) {
        m_dash_offset = 0.0;  // 超过一个周期后归零，循环滚动
    }

    update();   // 请求重绘以反映新的偏移
}

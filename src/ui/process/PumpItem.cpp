// ============================================================
// PumpItem.cpp — 泵图元实现
// ============================================================
//
// 实现 PumpItem 泵图元：在工艺流程图中表示一台输送泵。
// 外观为一个圆形泵体 + 内部旋转叶片（三角形），泵运行时叶片
// 持续旋转，泵体边框变为绿色；停止时为灰色，叶片不旋转。
// 泵体下方显示位号（如 P-101）。
// ============================================================

#include "PumpItem.h"     // 泵图元类声明
#include <QPainter>       // Qt 2D 绘制引擎

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 保存泵的位号，开启位图缓存，并设置悬浮提示文本。
// ------------------------------------------------------------
PumpItem::PumpItem(const QString& tag_name)
    : m_tag_name(tag_name)   // 记录位号（如 "P-101"）
{
    // 内容不变时缓存绘制结果为位图，减少重复绘制开销
    setCacheMode(DeviceCoordinateCache);

    // 设置鼠标悬浮提示（"泵 P-101" 等）
    setToolTip(QString::fromUtf8("泵 %1").arg(tag_name));
}

// ------------------------------------------------------------
// boundingRect() — 返回图元外接矩形
// ------------------------------------------------------------
// 以泵体中心为原点，四周各外扩 5 像素（容纳描边与抗锯齿边缘），
// 底部额外向下延伸 20 像素，用于显示下方的位号文字。
// ------------------------------------------------------------
QRectF PumpItem::boundingRect() const
{
    return QRectF(-RADIUS - 5, -RADIUS - 5,
        (RADIUS + 5) * 2, (RADIUS + 5) * 2 + 34);
}

// ------------------------------------------------------------
// paint() — 绘制泵图元
// ------------------------------------------------------------
// 绘制步骤：
//   1. 绘制圆形泵体（深色填充、彩色描边）；
//   2. 在泵体内部绘制叶片（三角形），并随 m_rotation 旋转；
//   3. 在泵体下方绘制位号文字。
// 泵运行时描边与叶片为绿色，停止时为灰色。
// ------------------------------------------------------------
void PumpItem::paint(QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget)
{
    Q_UNUSED(option);   // 本图元不使用样式选项
    Q_UNUSED(widget);   // 本图元不使用目标 widget

    // 开启抗锯齿，使圆形与叶片边缘更平滑
    painter->setRenderHint(QPainter::Antialiasing);

    // 根据运行状态选择颜色：运行 = 绿色，停止 = 灰色
    QColor color = m_running ? QColor("#2ECC71") : QColor("#7F8C8D");

    // 绘制圆形泵体
    painter->setPen(QPen(color, 2));            // 彩色描边
    painter->setBrush(QColor("#2C3E50"));       // 深色填充
    painter->drawEllipse(QPointF(0, 0), RADIUS, RADIUS); // 以原点为圆心

    // 保存当前变换，接下来在泵体内部绘制旋转叶片
    painter->save();

    // 绕泵体中心（原点）旋转指定角度，形成叶片旋转动画
    painter->rotate(m_rotation);

    // 构建叶片三角形（三个顶点：顶部一点、底部左右两点）
    QPolygonF triangle;
    triangle << QPointF(0, -RADIUS + 4)      // 顶部顶点（靠近上边缘）
        << QPointF(-RADIUS + 6, RADIUS - 6)  // 左下顶点
        << QPointF(RADIUS - 6, RADIUS - 6);  // 右下顶点

    painter->setBrush(color);   // 叶片填充色（与运行状态一致）
    painter->setPen(Qt::NoPen); // 叶片无描边
    painter->drawPolygon(triangle);

    // 恢复变换（取消旋转，避免影响后续文字绘制）
    painter->restore();

    // 在泵体下方绘制位号文字
    painter->setPen(QColor(Qt::black));
    QFont font = painter->font();
    font.setPointSize(6);
    painter->setFont(font);
    painter->drawText(QRectF(-25, RADIUS + 2, 50, 14),
        Qt::AlignCenter, m_tag_name);

    // 在泵体下方绘制中文标签（说明泵的作用）
    if (!m_label.isEmpty()) {
        painter->drawText(QRectF(-25, RADIUS + 16, 50, 14),
            Qt::AlignCenter, m_label);
    }
}

// ------------------------------------------------------------
// setRunning() — 设置泵运行状态
// ------------------------------------------------------------
// 状态变化时才更新并触发重绘（控制颜色与叶片是否旋转）。
// ------------------------------------------------------------
void PumpItem::setRunning(bool running)
{
    if (m_running != running) {
        m_running = running;
        update();   // 请求重绘
    }
}

// ------------------------------------------------------------
// setLabel() — 设置泵的中文标签
// ------------------------------------------------------------
void PumpItem::setLabel(const QString& text)
{
    m_label = text;
    update();
}

// ------------------------------------------------------------
// advanceAnimation() — 推进旋转动画一帧
// ------------------------------------------------------------
// 仅在泵运行时生效。每调用一次，叶片旋转角度加 15°；
// 达到 360° 后归零，形成连续旋转动画。由外部定时器周期调用。
// ------------------------------------------------------------
void PumpItem::advanceAnimation()
{
    if (!m_running) return;   // 停止状态不产生动画

    m_rotation += 15.0;       // 每次旋转 15°

    if (m_rotation >= 360.0) {
        m_rotation = 0.0;     // 转满一圈后归零
    }

    update();   // 请求重绘以反映新的旋转角度
}

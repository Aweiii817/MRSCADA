// ============================================================
// ReactorItem.cpp — 反应釜图元实现
// ============================================================
//
// 本文件实现 ReactorItem 图元类，用于在工艺流程图（ProcessView）
// 中绘制一个反应釜。它继承自 QGraphicsItem，通过重写 paint() 函数
// 自绘外观，并提供多个 setter 接口，供外部（ProcessView）实时更新
// 釜内温度、压力、液位、搅拌状态与工艺阶段等运行参数。
//
// 绘制坐标约定：
//   图元的本地坐标系以反应釜中心为原点 (0, 0)，
//   x 轴向右为正，y 轴向下为正（Qt 默认屏幕坐标方向）。
//   因此“向上”对应负 y 方向。
// ============================================================

#include "ReactorItem.h"                     // 反应釜图元类声明
#include <QPainter>                          // Qt 2D 绘制引擎
#include <QGraphicsSceneHoverEvent>          // 鼠标悬浮事件
#include <QToolTip>                          // 悬浮提示（ToolTip）

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 完成图元的基础配置：
//   1. 开启位图缓存，减少重复绘制开销，提升性能；
//   2. 开启悬浮事件接收，用于在鼠标悬停时弹出参数提示；
//   3. 设置一个默认的悬浮提示文本。
// ------------------------------------------------------------
ReactorItem::ReactorItem()
{
    // 启用缓存模式：当图元内容没有变化时，Qt 会把 paint 的结果
    // 缓存成一张位图，下次直接贴图而不重新执行绘制逻辑，大幅提升性能。
    // DeviceCoordinateCache：缓存按设备坐标存储，适合旋转/缩放后的图元。
    setCacheMode(DeviceCoordinateCache);

    // 允许接收鼠标悬浮事件（用于显示悬浮提示）
    setAcceptHoverEvents(true);

    // 设置默认的悬浮提示文本（鼠标悬停时显示）
    setToolTip(QString::fromUtf8("反应釜 R-101"));
}

// ------------------------------------------------------------
// boundingRect() — 返回图元的外接矩形
// ------------------------------------------------------------
// 该矩形决定了图元在场景中占据的范围，也决定了需要重绘的区域。
// 坐标基于本地坐标系（以图元中心为原点）。
//
// 说明：
//   - BODY_WIDTH / BODY_HEIGHT 为釜体宽高；
//   - 左右各向外扩 10，容纳描边与抗锯齿边缘；
//   - 顶部向上扩 30，用于显示上方的“工艺阶段”文字（m_stage_text）；
//   - 底部向下扩 40，留出下方文字余量。
// ------------------------------------------------------------
QRectF ReactorItem::boundingRect() const
{
    // 左侧额外留出空间，用于显示夹套的位号、中文标签与温度数值
    return QRectF(-BODY_WIDTH / 2 - JACKET_MARGIN - 100,
        -BODY_HEIGHT / 2 - 30,
        BODY_WIDTH + 2 * JACKET_MARGIN + 100 + 8,
        BODY_HEIGHT + 70);
}

// ------------------------------------------------------------
// paint() — 绘制图元
// ------------------------------------------------------------
// Qt 在需要重绘时调用此函数。整个反应釜的绘制分为以下几个步骤：
//   1. 绘制釜体（灰色金属圆角矩形，带渐变）；
//   2. 在釜体内部裁剪区域内绘制液位填充；
//   3. 绘制搅拌轴（搅拌运行时）；
//   4. 绘制数值文字（温度 / 压力 / 液位）与阶段文字。
// ------------------------------------------------------------
void ReactorItem::paint(QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget)
{
    Q_UNUSED(option);   // 本图元不使用 QStyleOptionGraphicsItem
    Q_UNUSED(widget);   // 本图元不使用目标 widget

    // 开启抗锯齿，使圆弧、斜线等边缘更平滑
    painter->setRenderHint(QPainter::Antialiasing);

    // 计算釜体尺寸与左上角坐标
    qreal w = BODY_WIDTH;          // 釜体宽度
    qreal h = BODY_HEIGHT;         // 釜体高度
    qreal x = -w / 2;              // 釜体左边界（因中心在原点）
    qreal y = -h / 2;              // 釜体上边界

    // 绘制加热夹套：包裹釜体外围的矩形虚线框
    // 加热时为红色循环平移虚线，未加热时为灰色静止虚线
    QRectF jacket_rect(x - JACKET_MARGIN, y - JACKET_MARGIN,
        w + 2 * JACKET_MARGIN, h + 2 * JACKET_MARGIN);
    QPen jacket_pen;
    jacket_pen.setStyle(Qt::DashLine);
    jacket_pen.setWidth(2);
    if (m_heating) {
        jacket_pen.setColor(QColor("#E74C3C"));            // 加热：红色
        jacket_pen.setDashOffset(m_jacket_dash_offset);    // 循环平移
    }
    else {
        jacket_pen.setColor(QColor("#95A5A6"));            // 未加热：灰色
    }
    painter->setPen(jacket_pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(jacket_rect);

    // 绘制釜体：浅蓝面板 + 蓝色描边
    painter->setPen(QPen(QColor("#4A90D9"), 3)); // 蓝色描边
    painter->setBrush(QColor("#D6EAF8"));        // 浅蓝填充
    painter->drawRoundedRect(QRectF(x, y, w, h), 20, 20); // 圆角釜体

    // 保存当前画笔状态，接下来对液位填充做局部裁剪
    painter->save();

    // 定义釜体内部的液位显示区域（比釜体略小，形成内边距）
    QRectF inner_rect(x + 4, y + 4, w - 8, h - 8);

    // 裁剪到内部区域，保证液位填充不会溢出釜体
    painter->setClipRect(inner_rect);

    // 计算液位占比（0~1），并换算成填充高度
    double level_ratio = qBound(0.0, m_level / 100.0, 1.0);
    qreal fill_h = inner_rect.height() * level_ratio;

    // 仅当液位大于 0 时才绘制填充
    if (fill_h > 0) {
        // 根据温度选择液体颜色：
        //   温度 > 120℃ 时用橙色（高温警示），否则用蓝色（常温）
        QColor liquid_color;
        if (m_temperature > 120.0) {
            liquid_color = QColor("#E67E22");   // 橙色（高温警示）
        }
        else {
            liquid_color = QColor("#2ECC71");   // 绿色（目标截图风格）
        }

        // 液位填充使用垂直渐变，上亮下暗，增强立体感
        QLinearGradient liquid_grad(0, inner_rect.bottom() - fill_h,
            0, inner_rect.bottom());
        liquid_grad.setColorAt(0.0, liquid_color.lighter(130)); // 顶部较亮
        liquid_grad.setColorAt(1.0, liquid_color.darker(120));  // 底部较暗

        painter->setPen(Qt::NoPen);             // 液面无描边
        painter->setBrush(liquid_grad);         // 渐变填充
        painter->drawRect(QRectF(inner_rect.left(),
            inner_rect.bottom() - fill_h,       // 从底部向上填充
            inner_rect.width(),
            fill_h));

        // 绘制液面分界线（半透明白色细线）
        painter->setPen(QPen(QColor(255, 255, 255, 180), 1));
        painter->drawLine(QPointF(inner_rect.left(),
            inner_rect.bottom() - fill_h),
            QPointF(inner_rect.right(),
                inner_rect.bottom() - fill_h));
    }

    // 恢复裁剪前的画笔状态
    painter->restore();

    // 搅拌轴：仅在搅拌运行时绘制
    if (m_agitator_running) {
        painter->setPen(QPen(QColor(255, 255, 255, 150), 3)); // 半透明白色
        // 竖轴：从釜体顶部向下延伸到釜体约 60% 高度
        painter->drawLine(QPointF(0, y + 10), QPointF(0, y + h * 0.6));
        // 底部搅拌桨叶（一条横线）
        painter->drawLine(QPointF(-25, y + h * 0.6),
            QPointF(25, y + h * 0.6));
    }

    // 配置字体（标签与数据文字颜色分别设置）
    QFont font = painter->font();
    font.setPointSize(7);
    font.setBold(true);
    painter->setFont(font);

    // 釜体上方显示"反应釜"标签（黑色）
    painter->setPen(QColor(Qt::black));
    font.setPointSize(8);
    painter->setFont(font);
    painter->drawText(QRectF(x, y - 24, w, 18),
        Qt::AlignCenter, QString::fromUtf8("反应釜"));
    font.setPointSize(7);
    painter->setFont(font);

    // 釜体内部数据显示（红色）
    painter->setPen(QColor("#E74C3C"));

    // 釜体内部显示温度（保留 1 位小数，单位 ℃）
    painter->drawText(QRectF(x + 5, y + 15, w - 10, 16),
        Qt::AlignCenter,
        QString("%1°C").arg(m_temperature, 0, 'f', 1));

    // 釜体内部显示压力（保留 2 位小数，单位 MPa）
    painter->drawText(QRectF(x + 5, y + 32, w - 10, 16),
        Qt::AlignCenter,
        QString("%1 MPa").arg(m_pressure, 0, 'f', 2));

    // 釜体中部显示液位百分比（保留 1 位小数）
    font.setPointSize(9);
    painter->setFont(font);
    painter->drawText(QRectF(x, y + h * 0.4, w, 20),
        Qt::AlignCenter,
        QString("%1%").arg(m_level, 0, 'f', 1));

    // 夹套位号与中文标签（显示在夹套左侧偏下）
    qreal jacket_left = x - JACKET_MARGIN;
    QFont jacket_font = painter->font();
    jacket_font.setPointSize(7);
    jacket_font.setBold(false);
    painter->setFont(jacket_font);
    painter->setPen(QColor(Qt::black));
    painter->drawText(QRectF(jacket_left - 6 - 90, 44, 90, 16),
        Qt::AlignRight | Qt::AlignVCenter,
        QString::fromUtf8("TE-102 加热夹套"));

    // 夹套温度数值（红色加粗）
    jacket_font.setPointSize(8);
    jacket_font.setBold(true);
    painter->setFont(jacket_font);
    painter->setPen(QColor("#E74C3C"));
    painter->drawText(QRectF(jacket_left - 6 - 90, 60, 90, 18),
        Qt::AlignRight | Qt::AlignVCenter,
        QString("%1°C").arg(m_jacket_temperature, 0, 'f', 1));
}

// ------------------------------------------------------------
// setLevel() — 设置液位（百分比 0~100）
// ------------------------------------------------------------
// 仅当新值与当前值差值超过 0.5 时才更新并触发重绘，
// 避免因微小波动频繁重绘造成性能浪费。
// ------------------------------------------------------------
void ReactorItem::setLevel(double percent)
{
    double new_level = qBound(0.0, percent, 100.0);
    if (qAbs(new_level - m_level) > 0.5) {
        m_level = new_level;
        update();   // 请求重绘
    }
}

// ------------------------------------------------------------
// setTemperature() — 设置釜内温度（℃）
// ------------------------------------------------------------
// 仅当差值超过 0.3℃ 时才更新并重绘。
// ------------------------------------------------------------
void ReactorItem::setTemperature(double celsius)
{
    if (qAbs(celsius - m_temperature) > 0.3) {
        m_temperature = celsius;
        update();
    }
}

// ------------------------------------------------------------
// setPressure() — 设置釜内压力（MPa）
// ------------------------------------------------------------
// 仅当差值超过 0.01 MPa 时才更新并重绘。
// ------------------------------------------------------------
void ReactorItem::setPressure(double mpa)
{
    if (qAbs(mpa - m_pressure) > 0.01) {
        m_pressure = mpa;
        update();
    }
}

// ------------------------------------------------------------
// setAgitatorRunning() — 设置搅拌运行状态
// ------------------------------------------------------------
// 状态变化时才更新并重绘（控制是否绘制搅拌轴）。
// ------------------------------------------------------------
void ReactorItem::setAgitatorRunning(bool running)
{
    if (m_agitator_running != running) {
        m_agitator_running = running;
        update();
    }
}

// ------------------------------------------------------------
// setStageText() — 设置工艺阶段文字
// ------------------------------------------------------------
// 文字变化时才更新并重绘（显示在釜体上方的橙色阶段文字）。
// ------------------------------------------------------------
void ReactorItem::setStageText(const QString& text)
{
    if (m_stage_text != text) {
        m_stage_text = text;
        update();
    }
}

// ------------------------------------------------------------
// setJacketTemperature() — 设置夹套温度（℃）
// ------------------------------------------------------------
// 仅当差值超过 0.3℃ 时才更新并重绘。
// ------------------------------------------------------------
void ReactorItem::setJacketTemperature(double celsius)
{
    if (qAbs(celsius - m_jacket_temperature) > 0.3) {
        m_jacket_temperature = celsius;
        update();
    }
}

// ------------------------------------------------------------
// setHeating() — 设置夹套是否处于加热状态
// ------------------------------------------------------------
// 状态变化时才更新并重绘（控制夹套矩形虚线的颜色与动画）。
// ------------------------------------------------------------
void ReactorItem::setHeating(bool heating)
{
    if (m_heating != heating) {
        m_heating = heating;
        update();
    }
}

// ------------------------------------------------------------
// advanceAnimation() — 推进夹套加热虚线动画一帧
// ------------------------------------------------------------
// 仅在加热状态下生效。每次调用使虚线偏移量加 1，达到一个
// 虚线周期（8 像素）后归零，形成沿夹套矩形循环平移的效果。
// ------------------------------------------------------------
void ReactorItem::advanceAnimation()
{
    if (!m_heating) return;

    m_jacket_dash_offset += 1.0;
    if (m_jacket_dash_offset > 8.0) {
        m_jacket_dash_offset = 0.0;
    }

    update();
}

// ------------------------------------------------------------
// hoverEnterEvent() — 鼠标悬浮进入事件
// ------------------------------------------------------------
// 当鼠标悬浮到图元上时，动态构建一个包含当前运行参数的
// 多行提示文本（温度 / 夹套温度 / 压力 / 液位 / 阶段），并设置为 ToolTip。
// ------------------------------------------------------------
void ReactorItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    setToolTip(QString::fromUtf8("反应釜 R-101\n"
        "温度: %1 °C\n"
        "夹套温度: %2 °C\n"
        "压力: %3 MPa\n"
        "液位: %4 %\n"
        "阶段: %5")
        .arg(m_temperature, 0, 'f', 1)       // %1 温度
        .arg(m_jacket_temperature, 0, 'f', 1) // %2 夹套温度
        .arg(m_pressure, 0, 'f', 2)           // %3 压力
        .arg(m_level, 0, 'f', 1)              // %4 液位
        .arg(m_stage_text));                  // %5 阶段

    // 调用基类实现，保持事件链完整
    QGraphicsItem::hoverEnterEvent(event);
}

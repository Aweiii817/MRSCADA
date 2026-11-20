// ============================================================
// StatusIndicator.cpp — LED 状态指示灯实现
// ============================================================
#include "StatusIndicator.h"
#include <QPainter>
#include <QTimerEvent>

StatusIndicator::StatusIndicator(QWidget* parent)
    : QWidget(parent)
{
	// 设置控件为透明背景，以便绘制圆形指示灯
    setAttribute(Qt::WA_TranslucentBackground);
}

// 设置指示灯标签文本
void StatusIndicator::setLabel(const QString& text)
{
    m_label = text;
    update();
}

// 设置指示灯状态
void StatusIndicator::setState(IndicatorState state)
{
    m_state = state;
	// 如果状态为 ALARM 且闪烁开启，则启动定时器
    if (state == IndicatorState::ALARM && m_blinking) {
        if (m_blink_timer_id == 0) {
            m_blink_timer_id = startTimer(500);
        }
    }
    else {
        if (m_blink_timer_id != 0) {
            killTimer(m_blink_timer_id);
            m_blink_timer_id = 0;
            m_blink_on = true; 
        }
    }

    update();
}

// 获取指示灯状态
IndicatorState StatusIndicator::state() const
{
    return m_state;
}

// 设置指示灯是否闪烁
void StatusIndicator::setBlinking(bool blink)
{
    m_blinking = blink;
}

// 建议控件尺寸
QSize StatusIndicator::sizeHint() const
{
    return QSize(120, 30);
}

QSize StatusIndicator::minimumSizeHint() const
{
    return QSize(80, 24);
}

// 根据当前状态获取对应的颜色
QColor StatusIndicator::stateColor() const
{
    switch (m_state) {
	case IndicatorState::ON:      return QColor("#2ECC71");  // 绿色
	case IndicatorState::WARNING: return QColor("#F39C12");  // 黄色
	case IndicatorState::ALARM:   return QColor("#E74C3C");  // 红色
    case IndicatorState::OFF:
	default:                      return QColor("#5D6D7E");  // 灰色
    }
}

// 处理闪烁定时器事件
void StatusIndicator::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_blink_timer_id) {
        m_blink_on = !m_blink_on;
        update();
    }
    QWidget::timerEvent(event);
}

// 重写绘制事件
void StatusIndicator::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
	// 开启抗锯齿
    painter.setRenderHint(QPainter::Antialiasing);
	// 计算指示灯圆形的直径，确保在控件高度范围内
    int diameter = qMin(height() - 4, 20);
    int cx = diameter / 2 + 2;        
    int cy = height() / 2;           

    QColor color = stateColor();
    if (m_state == IndicatorState::ALARM && m_blinking && !m_blink_on) {
        color = color.darker(200); 
    }
	// 创建径向渐变（圆心偏移以模拟3d效果）
    QRadialGradient gradient(cx - diameter / 4, cy - diameter / 4, diameter);
    gradient.setColorAt(0.0, color.lighter(150));  
    gradient.setColorAt(1.0, color.darker(120));  

    painter.setPen(QPen(color.darker(150), 1));   
    painter.setBrush(gradient);
    painter.drawEllipse(cx - diameter / 2, cy - diameter / 2,
        diameter, diameter);

    // 标签文字（黑色）
    painter.setPen(QColor(Qt::black));
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    painter.drawText(QRect(diameter + 8, 0,
        width() - diameter - 8, height()),
        Qt::AlignVCenter | Qt::AlignLeft, m_label);
}
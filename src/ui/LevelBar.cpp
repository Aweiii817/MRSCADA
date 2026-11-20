// ============================================================
// LevelBar.cpp — 液位柱实现
// ============================================================
#include "LevelBar.h"
#include <QPainter>

LevelBar::LevelBar(QWidget* parent)
    : QWidget(parent)
{
	// 设置控件为透明背景，以便绘制液位柱
    setAttribute(Qt::WA_TranslucentBackground);
}

// 设置量程范围
void LevelBar::setRange(double min_value, double max_value)
{
    m_min = min_value;
    m_max = max_value;
    update();
}

// 设置标题
void LevelBar::setTitle(const QString& title)
{
    m_title = title;
    update();
}

// 设置单位
void LevelBar::setUnit(const QString& unit)
{
    m_unit = unit;
    update();
}

// 设置高限值
void LevelBar::setHighLimit(double limit)
{
    m_high_limit = limit;
    m_has_limit = true;
    update();
}

// 设置当前值
void LevelBar::setValue(double value)
{
    m_value = qBound(m_min, value, m_max);
    update();
}

// 设置值是否有效
void LevelBar::setValid(bool valid)
{
    m_valid = valid;
    update();
}

// 建议控件尺寸
QSize LevelBar::sizeHint() const
{
    return QSize(80, 200);   
}

QSize LevelBar::minimumSizeHint() const
{
    return QSize(60, 120);
}

// 绘制液位柱
void LevelBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
	// 开启抗锯齿，提高绘制质量
    painter.setRenderHint(QPainter::Antialiasing);

    int title_h = 20;
    int value_h = 24;
	// 绘制液位柱背景（蓝色描边 + 浅蓝填充）
    QRect bar_rect(22, title_h,
        width() - 34, height() - title_h - value_h);
	// 如果液位柱高度小于10像素，则不绘制
    if (bar_rect.height() < 10) return; 

    painter.setPen(QPen(QColor("#4A90D9"), 1));
    painter.setBrush(QColor("#D6EAF8"));
    painter.drawRect(bar_rect);
	// 绘制液位柱填充部分
    if (m_valid) {
        double ratio = 0.0;
        if (m_max > m_min) {
            ratio = (m_value - m_min) / (m_max - m_min);
        }

        int fill_h = static_cast<int>(bar_rect.height() * ratio);
		// 绘制填充矩形，留出边框
        QRect fill_rect(bar_rect.left() + 1,
            bar_rect.bottom() - fill_h,
            bar_rect.width() - 1,
            fill_h);
		// 根据当前值是否超过高限值，选择填充颜色（目标截图风格：绿色）
        QColor fill_color = (m_has_limit && m_value >= m_high_limit)
            ? QColor("#E74C3C") : QColor("#2ECC71");

        QLinearGradient grad(fill_rect.topLeft(), fill_rect.bottomLeft());
        grad.setColorAt(0.0, fill_color.lighter(130));
        grad.setColorAt(1.0, fill_color.darker(110));

        painter.setPen(Qt::NoPen);
        painter.setBrush(grad);
        painter.drawRect(fill_rect);
    }
	// 绘制高限值虚线
    if (m_has_limit && m_max > m_min) {
        double limit_ratio = (m_high_limit - m_min) / (m_max - m_min);
        int limit_y = bar_rect.bottom()
            - static_cast<int>(bar_rect.height() * limit_ratio);

        painter.setPen(QPen(QColor("#E74C3C"), 2, Qt::DashLine)); 
        painter.drawLine(bar_rect.left() - 4, limit_y,
            bar_rect.right() + 4, limit_y);
    }
	// 绘制刻度线和刻度值（目标截图风格：白色刻度文字）
    painter.setPen(QColor("#ECF0F1"));
    QFont font = painter.font();
    font.setPointSize(6);
    painter.setFont(font);

    for (int i = 0; i <= 4; ++i) {
        int y = bar_rect.bottom() - bar_rect.height() * i / 4;
        painter.drawLine(bar_rect.left() - 6, y, bar_rect.left(), y);

        double tick_value = m_min + (m_max - m_min) * i / 4.0;
        painter.setPen(QColor("#ECF0F1"));
        painter.drawText(QRect(0, y - 6, 18, 12),
            Qt::AlignRight | Qt::AlignVCenter,
            QString::number(static_cast<int>(tick_value)));
    }
	// 绘制标题（黑色）
    painter.setPen(QColor(Qt::black));
    font.setPointSize(8);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, width(), title_h),
        Qt::AlignCenter, m_title);
	// 绘制当前值（红色高亮）
    font.setPointSize(9);
    painter.setFont(font);
    painter.setPen(QColor("#E74C3C"));
    QString value_text = m_valid
        ? QString::number(m_value, 'f', 1) + m_unit
        : "---";
    painter.drawText(QRect(0, height() - value_h, width(), value_h),
        Qt::AlignCenter, value_text);
}
// ============================================================
// GaugeWidget.cpp 圆形仪表盘实现
// ============================================================

#include "GaugeWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <cmath>

GaugeWidget::GaugeWidget(QWidget* parent)
	: QWidget(parent)
{
	// 设置控件为透明背景，以便绘制圆形仪表盘
	setAttribute(Qt::WA_TranslucentBackground);
}

// 设置量程范围
void GaugeWidget::setRange(double min_value, double max_value)
{
	m_min = min_value;
	m_max = max_value;
}

// 设置标题和副标题
void GaugeWidget::setTitle(const QString& title, const QString& subtitle)
{
	m_title = title;
	m_subtitle = subtitle;
	update();
}

// 设置单位
void GaugeWidget::setUnit(const QString& unit)
{
	m_unit = unit;
	update();
}

// 设置高限值
void GaugeWidget::setHighLimit(double high_limit)
{
	if (std::isnan(high_limit)) {
		m_has_limit = false;
	}
	else {
		m_high_limit = high_limit;
		m_has_limit = true;
	}
	update();
}

// 设置当前值
void GaugeWidget::setValue(double value)
{
	m_value = qBound(m_min, value, m_max);
	update();
}

// 设置值是否有效
void GaugeWidget::setValid(bool valid) {
	m_valid = valid;
	update();
}

// 获取当前值
double GaugeWidget::value() const
{
	return m_value;
}

// Qt建议控件尺寸
QSize GaugeWidget::sizeHint() const
{
	return QSize(200, 200);
}

// Qt建议控件最小尺寸
QSize GaugeWidget::minimumSizeHint() const
{
	return QSize(120, 120);
}

// 将“值”转换为“角度”
double GaugeWidget::valueToAngle(double value) const
{
	if (m_max <= m_min)return START_ANGLE;
	double ratio = (value - m_min) / (m_max - m_min);
	return START_ANGLE + ratio * SPAN_ANGLE;
}

// 重写绘制事件
void GaugeWidget::paintEvent(QPaintEvent* event)
{
	// 由于我们在paintEvent中使用了QPainter，所以不需要使用event参数
	Q_UNUSED(event);

	QPainter painter(this);
	// 开启抗锯齿
	painter.setRenderHint(QPainter::Antialiasing);
	// 将坐标系原点移动到控件中心，并根据控件大小进行缩放
	painter.translate(width() / 2.0, height() / 2.0);
	double scale = qMin(width(), height()) / 200.0;
	painter.scale(scale, scale);

	drawBackground(painter);
	drawTicks(painter);
	drawAlarmZone(painter);
	drawNeedle(painter);
	drawText(painter);
}

// 画背景（浅蓝表盘 + 灰色刻度环）
void GaugeWidget::drawBackground(QPainter& painter)
{
	// 表盘底色（浅蓝）
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor("#D6EAF8"));
	painter.drawEllipse(QPointF(0, 0), 92, 92);

	// 刻度环（灰色弧线）
	QPen pen(QColor("#7F8C8D"), 8);
	pen.setCapStyle(Qt::FlatCap);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	// 绘制圆弧
	painter.drawArc(QRectF(-90, -90, 180, 180), START_ANGLE * 16, SPAN_ANGLE * 16);
}

// 画刻度线和刻度文字（深色刻度）
void GaugeWidget::drawTicks(QPainter& painter)
{
	QPen pen(QColor("#2C3E50"), 2);
	painter.setPen(pen);

	for (int i = 0; i <= 10; ++i) {
		double value = m_min + (m_max - m_min) * i / 10.0;
		double angle = valueToAngle(value);
		// 计算角度对应的弧度（取反以匹配 drawArc/rotate 的角度方向）
		double rad = qDegreesToRadians(-angle);
		// 计算刻度线的起点和终点坐标
		double outer_x = 80 * qCos(rad);
		double outer_y = 80 * qSin(rad);
		double inner_x = 70 * qCos(rad);
		double inner_y = 70 * qSin(rad);
		// 绘制刻度线
		painter.drawLine(QPointF(inner_x, inner_y), QPointF(outer_x, outer_y));
		// 绘制刻度文字
		if (i % 2 == 0) {
			double text_r = 55;	// 文字在半径 55 的位置
			double tx = text_r * qCos(rad);
			double ty = text_r * qSin(rad);
			// 数字格式化（整数显示，不显示小数）
			QString text = QString::number(static_cast<int>(value));

			painter.setPen(QColor("#2C3E50"));
			QFont font = painter.font();
			font.setPointSize(7);
			painter.setFont(font);

			painter.drawText(QRectF(tx - 15, ty - 8, 30, 16), Qt::AlignCenter, text);
			painter.setPen(QPen(QColor("#2C3E50"), 2));
		}
	}
}

// 画报警区域
void GaugeWidget::drawAlarmZone(QPainter& painter)
{
	if (!m_has_limit) return;

	// 高限报警区：从报警值到量程最大值画红色粗弧
	if (m_high_limit >= m_max) return;

	double start_angle = valueToAngle(m_high_limit);
	double end_angle = START_ANGLE + SPAN_ANGLE;
	double span = end_angle - start_angle;

	QPen pen(QColor("#E74C3C"), 6);
	pen.setCapStyle(Qt::FlatCap);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	// 绘制报警区弧线
	painter.drawArc(QRectF(-90, -90, 180, 180), static_cast<int>(start_angle * 16), static_cast<int>(span * 16));
}

// 画指针
void GaugeWidget::drawNeedle(QPainter& painter)
{
	QColor needle_color;
	// 指针颜色：超过报警限变红，数据无效变灰
	if(!m_valid) {
		needle_color = QColor("#7F8C8D");
	} else if (m_has_limit && m_value >= m_high_limit) {
		needle_color = QColor("#E74C3C");
	} else {
		needle_color = QColor("#E67E22");	// 橙色指针（目标截图风格）
	}

	painter.save();

	double angle = valueToAngle(m_value);
	painter.rotate(angle);

	QPen pen(needle_color, 3);
	pen.setCapStyle(Qt::RoundCap);
	painter.setPen(pen);
	painter.drawLine(QPointF(0, 0), QPointF(65,0));

	painter.restore();
	// 画中心圆点（盖住指针根部）
	painter.setPen(Qt::NoPen);
	painter.setBrush(needle_color);
	painter.drawEllipse(QPointF(0, 0), 6, 6);
}

// 画数值和标题（标题黑色 + 数值红色）
void GaugeWidget::drawText(QPainter& painter)
{
	QFont font = painter.font();
	font.setPointSize(12);
	font.setBold(true);
	painter.setFont(font);

	QString value_text = m_valid ? QString::number(m_value, 'f', 1) + " " + m_unit : "---";

	painter.setPen(QColor("#E74C3C"));
	painter.drawText(QRectF(-50, 40, 100, 20), Qt::AlignCenter, value_text);

	font.setPointSize(9);
	font.setBold(false);
	painter.setFont(font);
	painter.setPen(QColor(Qt::black));
	painter.drawText(QRectF(-50, -45, 100, 16), Qt::AlignCenter, m_title);

	if(!m_subtitle.isEmpty()) {
		font.setPointSize(7);
		painter.setFont(font);
		painter.setPen(QColor(Qt::black));
		painter.drawText(QRectF(-50, -60, 100, 14), Qt::AlignCenter, m_subtitle);
	}
}
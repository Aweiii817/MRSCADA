// ============================================================
// GaugeWidget.h — 圆形仪表盘控件
// ============================================================

#pragma once

#include <QWidget>

class QPainter;

class GaugeWidget : public QWidget
{
	Q_OBJECT
public:
	explicit GaugeWidget(QWidget* parent = nullptr);

	// 设置量程范围
	void setRange(double min_value, double max_value);
	// 设置标题和副标题
	void setTitle(const QString& title, const QString& subtitle);
	// 设置单位
	void setUnit(const QString& unit);
	// 设置高限值
	void setHighLimit(double high_limit);
	// 设置当前值
	void setValue(double value);
	// 设置值是否有效
	void setValid(bool valid);
	// 获取当前值
	double value() const;
	// Qt建议控件尺寸
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

protected:
	// 重写绘制事件
	void paintEvent(QPaintEvent* event) override;
	// 画背景
	void drawBackground(QPainter& painter);
	// 画刻度线和刻度文字
	void drawTicks(QPainter& painter);
	// 画报警区域
	void drawAlarmZone(QPainter& painter);
	// 画指针
	void drawNeedle(QPainter& painter);
	// 画数值和标题
	void drawText(QPainter& painter);
	// 把“值”转换为“角度”
	double valueToAngle(double value) const;

	double m_min = 0.0;			// 量程最小值
	double m_max = 100.0;		// 量程最大值
	double m_high_limit = 0.0;	// 高限值
	bool m_has_limit = false;	// 是否设置了高限值

	QString m_title;		// 仪表盘标题-位号
	QString m_subtitle;		// 仪表盘副标题-描述
	QString m_unit;			// 仪表盘单位

	double m_value = 0.0;	// 当前值
	bool m_valid = true;	// 当前值是否有效

	static constexpr int START_ANGLE = -135;	// 起始角度
	static constexpr int SPAN_ANGLE = -270;	// 仪表盘总跨度角度（顺时针）
};
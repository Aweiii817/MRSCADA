// ============================================================
// LevelBar.h — 液位柱状显示控件
// ============================================================
#pragma once

#include <QWidget>

class LevelBar : public QWidget
{
    Q_OBJECT

public:
    explicit LevelBar(QWidget* parent = nullptr);
	// 设置量程范围
    void setRange(double min_value, double max_value);
	// 设置标题
    void setTitle(const QString& title);
	// 设置单位
    void setUnit(const QString& unit);
	// 设置高限值
    void setHighLimit(double limit);
	// 设置当前值
    void setValue(double value);
	// 设置值是否有效
    void setValid(bool valid);
	// 建议控件尺寸
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
	double m_min = 0.0;         // 量程最小值
	double m_max = 100.0;       // 量程最大值
	double m_value = 0.0;       // 当前值
	double m_high_limit = 90.0; // 高限值
	bool m_has_limit = true;    // 是否设置了高限值
	bool m_valid = true;        // 当前值是否有效
	QString m_title;			// 液位柱标题
	QString m_unit = "%";		// 液位柱单位
};
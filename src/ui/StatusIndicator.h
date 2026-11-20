// ============================================================
// StatusIndicator.h — LED 状态指示灯
//  状态决定颜色：运行(绿) / 停止(灰) / 报警(红) / 警告(黄)
// ============================================================
#pragma once

#include <QWidget>
#include <QString>

// 状态指示灯状态
enum class IndicatorState
{
	OFF,      // 停止状态，灰色
	ON,       // 运行状态，绿色
	WARNING,  // 警告状态，黄色
	ALARM     // 报警状态，红色
};

class StatusIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit StatusIndicator(QWidget* parent = nullptr);
	// 设置指示灯标签文本
    void setLabel(const QString& text);
	// 设置指示灯状态
    void setState(IndicatorState state);
	// 获取指示灯状态
    IndicatorState state() const;
	// 设置指示灯是否闪烁
    void setBlinking(bool blink);
	// 建议控件尺寸
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
	void timerEvent(QTimerEvent* event) override;   // 用于处理闪烁定时器事件

private:
	// 根据当前状态获取对应的颜色
    QColor stateColor() const;

	IndicatorState m_state = IndicatorState::OFF;   // 当前状态
	QString m_label;                                // 标签文本
	bool m_blinking = false;						// 是否闪烁
	bool m_blink_on = true;							// 当前闪烁状态，true表示亮，false表示灭
	int m_blink_timer_id = 0;						// 闪烁定时器ID
};
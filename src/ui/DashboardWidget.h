// ============================================================
// DashboardWidget.h — 反应釜实时仪表盘
// ============================================================
#pragma once

#include <QWidget>
#include <QTimer>
#include <string>

class GaugeWidget;
class LevelBar;
class StatusIndicator;

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget* parent = nullptr);

private slots:
    void refreshDisplay();

private:
    void setupUI();
    void updateValveLed(StatusIndicator* led, const std::string& tag_name);

    GaugeWidget* m_gauge_temp = nullptr;       // 釜内温度
    GaugeWidget* m_gauge_pressure = nullptr;   // 釜内压力
    LevelBar* m_level_bar = nullptr;           // 釜内液位

    // 原料罐仪表盘（一行 6 个：液位柱 + 温度/压力表盘）
    LevelBar* m_tank_a_level = nullptr;         // 原料罐A液位
    GaugeWidget* m_tank_a_temp = nullptr;       // 原料罐A温度
    GaugeWidget* m_tank_a_pressure = nullptr;   // 原料罐A压力
    LevelBar* m_tank_b_level = nullptr;         // 原料罐B液位
    GaugeWidget* m_tank_b_temp = nullptr;       // 原料罐B温度
    GaugeWidget* m_tank_b_pressure = nullptr;   // 原料罐B压力

    StatusIndicator* m_led_valve_a = nullptr;      // HV-101 进料A阀
    StatusIndicator* m_led_valve_b = nullptr;      // HV-102 进料B阀
    StatusIndicator* m_led_valve_out = nullptr;    // HV-103 出料阀

    QTimer* m_refresh_timer = nullptr;
};

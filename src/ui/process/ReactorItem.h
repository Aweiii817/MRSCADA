// ============================================================
// ReactorItem.h — 反应釜图元
// ============================================================
#pragma once

#include <QGraphicsItem>
#include <QString>

class ReactorItem : public QGraphicsItem
{
public:
    explicit ReactorItem();

    QRectF boundingRect() const override;
    void paint(QPainter* painter,
        const QStyleOptionGraphicsItem* option,
        QWidget* widget) override;

    // 设置液位
    void setLevel(double percent);     
    // 设置温度
    void setTemperature(double celsius);    
    // 设置压力
    void setPressure(double mpa);      
    // 设置搅拌运行状态
    void setAgitatorRunning(bool running);  
    // 设置当前工艺阶段文字
    void setStageText(const QString& text); 
    // 设置夹套温度（℃）
    void setJacketTemperature(double celsius);
    // 设置夹套是否处于加热状态（加热=红色流动虚线，未加热=灰色静止虚线）
    void setHeating(bool heating);
    // 推进夹套加热虚线动画一帧（由外部定时器周期调用）
    void advanceAnimation();

protected:
    // 重写悬浮事件，用于显示详细信息提示
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

private:
    double m_level = 0.0;          // 液位  
    double m_temperature = 25.0;   // 温度
    double m_pressure = 0.1;       // 压力
    double m_jacket_temperature = 25.0;  // 夹套温度
    bool m_agitator_running = false;    // 搅拌机运行状态
    bool m_heating = false;             // 夹套是否处于加热状态
    qreal m_jacket_dash_offset = 0.0;   // 夹套虚线偏移量（用于加热流动动画）
    QString m_stage_text = QString::fromUtf8("待机");    // 当前工艺阶段文字

    // 反应釜尺寸
    static constexpr qreal BODY_WIDTH = 120;
    static constexpr qreal BODY_HEIGHT = 160;
    // 加热夹套与釜体外壁之间的间距
    static constexpr qreal JACKET_MARGIN = 10.0;
};
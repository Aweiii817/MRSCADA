// ============================================================
// ProcessView.cpp — 工艺流程图实现
// ============================================================
//
// 实现 ProcessView 工艺流程图视图。负责：
//   1. 创建并布局各工艺图元（反应釜、原料罐、泵、阀门、管道）；
//   2. 周期读取实时数据库，刷新图元的运行状态与显示；
//   3. 周期推进流动/旋转动画；
//   4. 处理滚轮缩放与拖动平移。
// ============================================================

#include "ProcessView.h"
#include "process/ReactorItem.h"   // 反应釜图元
#include "process/TankItem.h"       // 原料罐图元
#include "process/PipeItem.h"      // 管道图元
#include "process/ValveItem.h"     // 阀门图元
#include "process/PumpItem.h"      // 泵图元
#include "rtdb/RealTimeDB.h"       // 实时数据库
#include "SimEngine.h"             // 仿真引擎（定义 TagDataPoint 等）
#include "Logger.h"                // 日志工具

#include <QGraphicsScene>          // 图形场景
#include <QGraphicsTextItem>       // 文字图元（罐体标签）
#include <QWheelEvent>             // 滚轮事件
#include <QResizeEvent>            // 尺寸变化事件

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 依次完成：场景初始化 → 图元创建 → 布局，然后配置视图渲染与
// 交互属性，最后启动数据刷新定时器与动画定时器。
// ------------------------------------------------------------
ProcessView::ProcessView(QWidget* parent)
    : QGraphicsView(parent)
{
    setupScene();      // 创建图形场景
    createItems();     // 创建各图元
    layoutItems();     // 布置图元位置

    // 开启抗锯齿渲染
    setRenderHint(QPainter::Antialiasing);

    // 拖动模式：按住鼠标左键可拖动场景（平移视图）
    setDragMode(ScrollHandDrag);

    // 缩放锚点：缩放时以鼠标所在位置为中心，便于局部放大
    setTransformationAnchor(AnchorUnderMouse);

    // 浅灰背景（与监控页面统一）
    setBackgroundBrush(QColor("#F0F0F0"));

    // 数据刷新定时器：每 200ms 读取一次实时数据
    m_data_timer = new QTimer(this);
    connect(m_data_timer, &QTimer::timeout,
        this, &ProcessView::refreshData);
    m_data_timer->start(200);

    // 动画定时器：每 50ms 推进一帧流动/旋转动画
    m_anim_timer = new QTimer(this);
    connect(m_anim_timer, &QTimer::timeout,
        this, &ProcessView::advanceAnimations);
    m_anim_timer->start(50);

    LOG_INFO("ProcessView created");
}

// ------------------------------------------------------------
// setupScene() — 初始化场景
// ------------------------------------------------------------
// 创建一个 800×600 的图形场景并设置为当前视图的场景。
// ------------------------------------------------------------
void ProcessView::setupScene()
{
    m_scene = new QGraphicsScene(0, 0, 800, 600, this);
    setScene(m_scene);
}

// ------------------------------------------------------------
// createItems() — 创建图元
// ------------------------------------------------------------
// 创建所有工艺图元并加入场景：
//   - 反应釜（ReactorItem）
//   - 两个原料罐（矩形）
//   - 两个罐体文字标签
//   - 两台泵、六个阀门
//   - 七段管道（A/B 进料、反应釜进料、出料、蒸汽、冷却水、泄压）
// 各图元的最终位置在 layoutItems() 中统一设置。
// ------------------------------------------------------------
void ProcessView::createItems()
{
    // 反应釜
    m_reactor = new ReactorItem();
    m_scene->addItem(m_reactor);

    // 两个原料罐（支持液位 / 温度 / 压力显示）
    m_tank_a = new TankItem();
    m_scene->addItem(m_tank_a);
    m_tank_b = new TankItem();
    m_scene->addItem(m_tank_b);

    // 原料罐 A 文字标签（黑色）
    auto* label_a = m_scene->addText(QString::fromUtf8("原料罐A"));
    label_a->setDefaultTextColor(QColor(Qt::black));
    label_a->setPos(40, 90);

    // 原料罐 B 文字标签
    auto* label_b = m_scene->addText(QString::fromUtf8("原料罐B"));
    label_b->setDefaultTextColor(QColor(Qt::black));
    label_b->setPos(40, 350);

    // 两台进料泵
    m_pump_a = new PumpItem("P-101");
    m_pump_a->setLabel(QString::fromUtf8("进料泵A"));
    m_scene->addItem(m_pump_a);
    m_pump_b = new PumpItem("P-102");
    m_pump_b->setLabel(QString::fromUtf8("进料泵B"));
    m_scene->addItem(m_pump_b);

    // 六个阀门（进料阀 ×2、出料阀、蒸汽调节阀、冷却水调节阀、泄压阀）
    m_valve_hv101 = new ValveItem("HV-101");
    m_valve_hv101->setLabel(QString::fromUtf8("进料阀A"));
    m_scene->addItem(m_valve_hv101);
    m_valve_hv102 = new ValveItem("HV-102");
    m_valve_hv102->setLabel(QString::fromUtf8("进料阀B"));
    m_scene->addItem(m_valve_hv102);
    m_valve_hv103 = new ValveItem("HV-103");
    m_valve_hv103->setLabel(QString::fromUtf8("出料阀"));
    m_scene->addItem(m_valve_hv103);

    // 蒸汽调节阀 TV-101（加热夹套）
    m_valve_tv101 = new ValveItem("TV-101");
    m_valve_tv101->setLabel(QString::fromUtf8("蒸汽调节阀"));
    m_scene->addItem(m_valve_tv101);
    // 冷却水调节阀 TV-102（冷却夹套）
    m_valve_tv102 = new ValveItem("TV-102");
    m_valve_tv102->setLabel(QString::fromUtf8("冷却水调节阀"));
    m_scene->addItem(m_valve_tv102);
    // 泄压阀 PV-101（釜顶泄压）
    m_valve_pv101 = new ValveItem("PV-101");
    m_valve_pv101->setLabel(QString::fromUtf8("泄压阀"));
    m_scene->addItem(m_valve_pv101);

    // 管道：原料罐 A → 泵/阀 → 直角弯折汇入进料总管
    // 水平段 y=160 由罐 A 右侧延伸，到 x=320 向下直角弯折至总管 y=290。
    m_pipe_feed_a = new PipeItem(QVector<QPointF>{
        QPointF(100, 160),   // 罐 A 右侧出口
        QPointF(320, 160),   // 水平段（经泵 A、阀 HV-101）
        QPointF(320, 290)    // 直角弯折向下，汇入进料总管
    });
    m_scene->addItem(m_pipe_feed_a);

    // 管道：原料罐 B → 泵/阀 → 直角弯折汇入进料总管
    // 水平段 y=420 由罐 B 右侧延伸，到 x=320 向上直角弯折至总管 y=290。
    m_pipe_feed_b = new PipeItem(QVector<QPointF>{
        QPointF(100, 420),   // 罐 B 右侧出口
        QPointF(320, 420),   // 水平段（经泵 B、阀 HV-102）
        QPointF(320, 290)    // 直角弯折向上，汇入进料总管
    });
    m_scene->addItem(m_pipe_feed_b);

    // 管道：进料总管 → 反应釜（水平进入釜体左侧）
    m_pipe_inlet = new PipeItem(QPointF(320, 290), QPointF(440, 290));
    m_scene->addItem(m_pipe_inlet);

    // 管道：反应釜 → 出料管路（底部垂直下落后直角弯折向右）
    m_pipe_outlet = new PipeItem(QVector<QPointF>{
        QPointF(500, 370),   // 釜体底部出口
        QPointF(500, 480),   // 垂直下落（经出料阀 HV-103）
        QPointF(640, 480)    // 直角弯折向右
    });
    m_scene->addItem(m_pipe_outlet);

    // 蒸汽管线：从反应釜右侧上部引出，经蒸汽调节阀 TV-101
    m_pipe_steam = new PipeItem(QPointF(560, 250), QPointF(680, 250));
    m_scene->addItem(m_pipe_steam);

    // 冷却水管线：从反应釜右侧下部引出，经冷却水调节阀 TV-102
    m_pipe_cooling = new PipeItem(QPointF(560, 330), QPointF(680, 330));
    m_scene->addItem(m_pipe_cooling);

    // 泄压管线：从反应釜顶部引出，竖直向上后向右排放
    m_pipe_relief = new PipeItem(QVector<QPointF>{
        QPointF(500, 210),   // 釜顶出口
        QPointF(500, 140),   // 竖直上升
        QPointF(640, 140)    // 向右排放
    });
    m_scene->addItem(m_pipe_relief);
}

// ------------------------------------------------------------
// layoutItems() — 布置图元位置
// ------------------------------------------------------------
// 为各图元设置最终场景坐标，然后调用 fitInView 自适应缩放，
// 使整个场景完整显示在视图内。
// ------------------------------------------------------------
void ProcessView::layoutItems()
{
    // 原料罐位置（左侧上下排列，坐标为罐体中心）
    m_tank_a->setPos(70, 160);
    m_tank_b->setPos(70, 420);

    // 泵位置（位于各自进料水平管线上）
    m_pump_a->setPos(150, 160);
    m_pump_b->setPos(150, 420);

    // 阀门位置（进料阀位于水平管线，出料阀位于出料垂直管线）
    m_valve_hv101->setPos(230, 160);
    m_valve_hv102->setPos(230, 420);
    m_valve_hv103->setPos(500, 420);

    // 蒸汽/冷却水/泄压阀门位置（位于对应管线上）
    m_valve_tv101->setPos(620, 250);   // 蒸汽调节阀（蒸汽管线）
    m_valve_tv102->setPos(620, 330);   // 冷却水调节阀（冷却水管线）
    m_valve_pv101->setPos(570, 140);   // 泄压阀（泄压管线水平段）

    // 反应釜位置（场景右侧中部）
    m_reactor->setPos(500, 290);

    // 根据所有图元的实际包围盒设置场景范围（四周留少量边距），
    // 使工艺流程图在视图中居中显示，且图元大小合适。
    QRectF items_rect = m_scene->itemsBoundingRect();
    m_scene->setSceneRect(items_rect.adjusted(-25, -25, 25, 25));

    // 自适应缩放，保持宽高比，使整个场景完整可见
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

// ------------------------------------------------------------
// refreshData() — 定时刷新图元数据
// ------------------------------------------------------------
// 每 200ms 调用一次。从实时数据库读取相关位号数据，更新：
//   - 反应釜的液位 / 温度 / 压力 / 搅拌状态
//   - 三个阀门的开闭状态
//   - 管道的流动状态（根据进料阀、出料阀开闭）
//   - 泵的运行状态
//   - 反应釜的工艺阶段文字
// ------------------------------------------------------------
void ProcessView::refreshData()
{
    auto& rtdb = RealTimeDB::instance();
    TagDataPoint pt;

    // 反应釜参数
    if (rtdb.getTagValue("LT-101", pt))
        m_reactor->setLevel(pt.scaled_value);          // 液位
    if (rtdb.getTagValue("TE-101", pt))
        m_reactor->setTemperature(pt.scaled_value);    // 温度
    if (rtdb.getTagValue("TE-102", pt))
        m_reactor->setJacketTemperature(pt.scaled_value); // 夹套温度
    if (rtdb.getTagValue("PT-101", pt))
        m_reactor->setPressure(pt.scaled_value);       // 压力
    if (rtdb.getTagValue("TT-101", pt))
        m_reactor->setAgitatorRunning(pt.scaled_value > 1.0); // 搅拌转速 > 1 视为运行

    // 原料罐 A 参数（液位 / 温度 / 压力）
    if (rtdb.getTagValue("LT-201", pt))
        m_tank_a->setLevel(pt.scaled_value);
    if (rtdb.getTagValue("TE-201", pt))
        m_tank_a->setTemperature(pt.scaled_value);
    if (rtdb.getTagValue("PT-201", pt))
        m_tank_a->setPressure(pt.scaled_value);

    // 原料罐 B 参数（液位 / 温度 / 压力）
    if (rtdb.getTagValue("LT-202", pt))
        m_tank_b->setLevel(pt.scaled_value);
    if (rtdb.getTagValue("TE-202", pt))
        m_tank_b->setTemperature(pt.scaled_value);
    if (rtdb.getTagValue("PT-202", pt))
        m_tank_b->setPressure(pt.scaled_value);

    // 阀门开闭状态（用 lambda 统一处理三个阀门）
    auto updateValve = [&rtdb](ValveItem* valve, const std::string& tag) {
        TagDataPoint p;
        if (rtdb.getTagValue(tag, p))
            valve->setOpen(p.scaled_value > 0.5);   // 数值 > 0.5 视为开启
        };
    updateValve(m_valve_hv101, "HV-101");
    updateValve(m_valve_hv102, "HV-102");
    updateValve(m_valve_hv103, "HV-103");
    updateValve(m_valve_tv101, "TV-101");
    updateValve(m_valve_tv102, "TV-102");
    updateValve(m_valve_pv101, "PV-101");

    // 判断进料/出料状态
    bool feeding = false, discharging = false;
    if (rtdb.getTagValue("HV-101", pt) && pt.scaled_value > 0.5) feeding = true;     // 进料阀 HV-101 开启
    if (rtdb.getTagValue("HV-103", pt) && pt.scaled_value > 0.5) discharging = true; // 出料阀 HV-103 开启

    // 管道流动状态：进料时相关进料管道流动，出料时出料管道流动
    m_pipe_feed_a->setFlowing(feeding);
    m_pipe_feed_b->setFlowing(feeding);
    m_pipe_inlet->setFlowing(feeding);
    m_pipe_outlet->setFlowing(discharging);

    // 泵运行状态：进料时泵运行
    m_pump_a->setRunning(feeding);
    m_pump_b->setRunning(feeding);

    // 蒸汽/冷却水/泄压管线流动状态（对应阀门开启时流动）
    bool steam_open = false, cooling_open = false, relief_open = false;
    if (rtdb.getTagValue("TV-101", pt) && pt.scaled_value > 0.5) steam_open = true;      // 蒸汽调节阀开启
    if (rtdb.getTagValue("TV-102", pt) && pt.scaled_value > 0.5) cooling_open = true;    // 冷却水调节阀开启
    if (rtdb.getTagValue("PV-101", pt) && pt.scaled_value > 0.5) relief_open = true;     // 泄压阀开启

    m_pipe_steam->setFlowing(steam_open);
    m_pipe_cooling->setFlowing(cooling_open);
    m_pipe_relief->setFlowing(relief_open);

    // 夹套加热状态：蒸汽调节阀开启即视为加热
    m_reactor->setHeating(steam_open);

    // 工艺阶段判定（优先级从高到低）
    TagDataPoint lv, tp;
    if (rtdb.getTagValue("LT-101", lv) && rtdb.getTagValue("TE-101", tp)) {
        QString stage;
        if (discharging)      stage = QString::fromUtf8("出料");
        else if (feeding)     stage = QString::fromUtf8("进料");
        else if (tp.scaled_value > 140.0) stage = QString::fromUtf8("恒温反应");
        else if (tp.scaled_value > 60.0)  stage = QString::fromUtf8("加热/冷却");
        else if (lv.scaled_value > 5.0)   stage = QString::fromUtf8("待机(有料)");
        else                  stage = QString::fromUtf8("待机");
        m_stage_text = stage;              // 保存供监控页面参数区显示
        m_reactor->setStageText(stage);    // 同步到反应釜（悬浮提示用）
    }
}

// ------------------------------------------------------------
// advanceAnimations() — 推进所有动画一帧
// ------------------------------------------------------------
// 每 50ms 调用一次，推进管道流动动画与泵叶片旋转动画。
// ------------------------------------------------------------
void ProcessView::advanceAnimations()
{
    m_pipe_feed_a->advanceAnimation();
    m_pipe_feed_b->advanceAnimation();
    m_pipe_inlet->advanceAnimation();
    m_pipe_outlet->advanceAnimation();
    m_pipe_steam->advanceAnimation();
    m_pipe_cooling->advanceAnimation();
    m_pipe_relief->advanceAnimation();
    m_pump_a->advanceAnimation();
    m_pump_b->advanceAnimation();
    m_reactor->advanceAnimation();
}

// ------------------------------------------------------------
// wheelEvent() — 鼠标滚轮缩放事件
// ------------------------------------------------------------
// 滚轮向上放大（×1.15），向下缩小（÷1.15），并将缩放限制在
// 0.3~5.0 倍之间，防止过度放大或缩小导致图元无法辨认。
// ------------------------------------------------------------
void ProcessView::wheelEvent(QWheelEvent* event)
{
    // 用户手动滚轮缩放后，关闭自动适配，避免缩放被覆盖
    m_auto_fit = false;

    // 根据滚轮方向确定缩放因子
    qreal factor = (event->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;

    // 获取当前缩放倍数（变换矩阵左上角 m11 即水平缩放系数）
    qreal current_scale = transform().m11();
    qreal new_scale = current_scale * factor;

    // 仅在缩放范围 [0.3, 5.0] 内允许缩放
    if (new_scale >= 0.3 && new_scale <= 5.0) {
        scale(factor, factor);
    }
}

// ------------------------------------------------------------
// resizeEvent() — 视图尺寸变化事件
// ------------------------------------------------------------
// 在自动适配模式下，每次视图尺寸变化都重新 fitInView，
// 使工艺流程图随窗口大小自适应缩放并保持居中显示。
// 用户手动滚轮缩放后会关闭自动适配（m_auto_fit = false）。
// ------------------------------------------------------------
void ProcessView::resizeEvent(QResizeEvent* event)
{
    // 先调用基类实现，保证 QGraphicsView 内部布局正确
    QGraphicsView::resizeEvent(event);

    // 自动适配模式下，重新缩放以填满视图（保持宽高比）
    if (m_auto_fit && m_scene) {
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

// ------------------------------------------------------------
// currentStageText() — 获取当前工艺阶段文字
// ------------------------------------------------------------
// 返回 refreshData() 中计算出的当前工艺阶段文字。
// ------------------------------------------------------------
QString ProcessView::currentStageText() const
{
    return m_stage_text;
}

// ============================================================
// main.cpp — 程序入口
// ============================================================
//
// 应用启动入口，负责按顺序完成以下初始化工作：
//   1. 创建 QApplication 应用对象并设置应用名称/版本；
//   2. 初始化日志系统（spdlog），并接管 Qt 的日志输出；
//   3. 加载深色主题样式表（QSS）；
//   4. 初始化配置管理器、用户管理器；
//   5. 加载默认位号定义（用于测试/演示）；
//   6. 初始化实时数据库；
//   7. 创建并显示主窗口；
//   8. 创建仿真引擎并注册到通信管理器，启动数据采集；
//   9. 进入 Qt 事件循环。
// ============================================================

#include <QApplication>                      // Qt 应用类
#include <QFile>                             // 文件读写（加载 QSS 样式）
#include "Logger.h"                          // 日志系统（spdlog 封装）
#include "ConfigManager.h"                   // 配置管理器
#include "tag/TagManager.h"                  // 位号管理器
#include "rtdb/RealTimeDB.h"                 // 实时数据库
#include "business/user/UserManager.h"       // 用户管理器
#include "MainWindow.h"                      // 主窗口
#include "simulation/SimEngine.h"            // 仿真引擎
#include "simulation/SimPort.h"              // 仿真通信端口
#include "comm/CommManager.h"                // 通信管理器
#include "core/alarm/AlarmEngine.h"
#include "business/recipe/RecipeManager.h"

// ------------------------------------------------------------
// installQtMessageHandler() — 接管 Qt 日志输出
// ------------------------------------------------------------
// 通过 qInstallMessageHandler 将 Qt 的 qDebug/qInfo/qWarning 等
// 日志重定向到 spdlog，使 Qt 内部日志与业务日志统一输出。
// 日志级别映射：
//   QtDebugMsg    → DEBUG
//   QtInfoMsg     → INFO
//   QtWarningMsg  → WARN（附文件名与行号）
//   QtCriticalMsg → ERROR（附文件名与行号）
//   QtFatalMsg    → ERROR，并调用 abort() 终止进程
// ------------------------------------------------------------
void installQtMessageHandler()
{
    qInstallMessageHandler([](QtMsgType type,
        const QMessageLogContext& ctx,
        const QString& msg) {
            switch (type) {
            case QtDebugMsg:    LOG_DEBUG("[Qt] {}", msg.toStdString()); break;
            case QtInfoMsg:     LOG_INFO("[Qt] {}", msg.toStdString()); break;
            case QtWarningMsg:  LOG_WARN("[Qt] {} ({}:{})", msg.toStdString(), ctx.file ? ctx.file : "?", ctx.line); break;
            case QtCriticalMsg: LOG_ERROR("[Qt] {} ({}:{})", msg.toStdString(), ctx.file ? ctx.file : "?", ctx.line); break;
            case QtFatalMsg:    LOG_ERROR("[Qt FATAL] {} ({}:{})", msg.toStdString(), ctx.file ? ctx.file : "?", ctx.line); abort();
            }
        });
}

// ------------------------------------------------------------
// loadDefaultTags() — 加载默认位号定义
// ------------------------------------------------------------
// 以 JSON 字符串形式内置一批默认位号，用于测试/演示。
// 通过 TagManager 的 loadFromJson() 解析并注册这些位号。
// 说明：数据已直接使用 UTF-8 中文字符（JSON 标准不支持 \x 转义）。
// ------------------------------------------------------------
void loadDefaultTags()
{
    // 默认位号列表（原始字符串字面量 R"(...)" 保持内容原样）
    std::string tags_json = R"({
        "tags": [
            {"tag_name":"TE-101","description":"反应器温度","unit":"℃","data_type":5,"source_device":"PLC-01","source_address":100},
            {"tag_name":"PT-101","description":"反应器压力","unit":"MPa","data_type":5,"source_device":"PLC-01","source_address":102},
            {"tag_name":"LT-101","description":"反应器液位","unit":"%","data_type":5,"source_device":"PLC-01","source_address":104},
            {"tag_name":"HV-101","description":"进料阀A","unit":"","data_type":0,"source_device":"PLC-01","source_address":200},
            {"tag_name":"LT-201","description":"原料罐A液位","unit":"%","data_type":5,"source_device":"PLC-01","source_address":300},
            {"tag_name":"TE-201","description":"原料罐A温度","unit":"℃","data_type":5,"source_device":"PLC-01","source_address":302},
            {"tag_name":"PT-201","description":"原料罐A压力","unit":"MPa","data_type":5,"source_device":"PLC-01","source_address":304},
            {"tag_name":"LT-202","description":"原料罐B液位","unit":"%","data_type":5,"source_device":"PLC-01","source_address":306},
            {"tag_name":"TE-202","description":"原料罐B温度","unit":"℃","data_type":5,"source_device":"PLC-01","source_address":308},
            {"tag_name":"PT-202","description":"原料罐B压力","unit":"MPa","data_type":5,"source_device":"PLC-01","source_address":310}
        ]
    })";
    TagManager::instance().loadFromJson(tags_json);
}

// ------------------------------------------------------------
// main() — 程序入口
// ------------------------------------------------------------
int main(int argc, char* argv[])
{
    // 创建 QApplication（必须先于任何 Qt 控件创建）
    QApplication app(argc, argv);
    app.setApplicationName("UCS");
    app.setApplicationVersion("1.0.0");

    // 初始化日志系统（输出目录 logs，级别 debug）
    Logger::initialize("logs", "debug");
    installQtMessageHandler();

    // 打印启动横幅
    LOG_INFO("============================");
    LOG_INFO("UCS Starting... v{}", app.applicationVersion().toStdString());
    LOG_INFO("============================");

    // 加载深色主题样式表（从 Qt 资源文件读取 QSS）
    QFile styleFile(":/styles/dark_theme.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
        LOG_INFO("Dark theme loaded");
    }

    // 初始化配置管理器（读取 config 目录下的配置）
    ConfigManager::instance().initialize("config");

    // 初始化用户管理器（使用 SQLite 数据库存储用户）
    UserManager::instance().initialize("config/users.db");

    // 加载默认位号（用于测试/演示）
    loadDefaultTags();

    // 初始化实时数据库（单例，首次访问即创建）
    RealTimeDB::instance();

	// ==== 初始化报警引擎 ====
	// 从资源文件读取报警规则
	QFile ruleFile(":/configs/alarm_rules.json");
	if (ruleFile.open(QFile::ReadOnly)) {
		AlarmEngine::instance().loadRulesFromJson(
			ruleFile.readAll().toStdString());
		ruleFile.close();
	}
	AlarmEngine::instance().start(200);   // 每 200ms 评估一次
	LOG_INFO("AlarmEngine started");

	// ==== 初始化配方管理器 ====
	RecipeManager::instance().initialize("config/recipes");

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.setWindowTitle(QString::fromUtf8("UCS 工业上位机监控系统 v1.0"));
    mainWindow.resize(1280, 1400);
    mainWindow.show();

    LOG_INFO("MainWindow displayed");

    // 创建仿真引擎，以 100ms 周期循环产生仿真数据
    auto* sim_engine = new SimEngine();
    sim_engine->start(100);

    // 将仿真引擎包装为通信端口（SimPort），注册到通信管理器
    auto sim_port = std::make_unique<SimPort>(sim_engine);
    DeviceConfig sim_config;
    sim_config.device_name = "SimEngine";
    sim_config.protocol_type = "simulation";
    sim_config.poll_interval_ms = 100;
    CommManager::instance().addDevice(std::move(sim_port), sim_config);
    CommManager::instance().startAll();

    LOG_INFO("Simulation engine started");

    // 进入 Qt 事件循环（阻塞，直到窗口关闭）
    return app.exec();
}

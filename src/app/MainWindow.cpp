// ============================================================
// MainWindow.cpp — 主窗口实现
// ============================================================
#include "MainWindow.h"
#include "LoginDialog.h"
#include "Logger.h"
#include "ConfigManager.h"
#include "business/user/UserManager.h"
#include "ui/MonitorWidget.h"
#include "CommManager.h"
#include "ui/DataAnalysisWidget.h"
#include "ui/alarm/AlarmListWidget.h"
#include "ui/recipe/RecipeEditorWidget.h"
#include "ui/report/ReportWidget.h"
#include "ui/log/LogViewWidget.h"

#include <QMenuBar>         
#include <QMenu>            
#include <QAction>          
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>      
#include <QCloseEvent>      
#include <QTimer>          
#include <QPushButton>
#include <QDateTime>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUI();
    updateUserStatus();

    // 适当扩大窗口并设置最小尺寸，确保工艺流程图能完整、合理地显示
    resize(1760, 1400);
    setMinimumSize(1600, 1400);

    // 连接用户管理信号
    connect(&UserManager::instance(), &UserManager::loginSucceeded,
        this, &MainWindow::onLoginSucceeded);
    connect(&UserManager::instance(), &UserManager::loggedOut,
        this, &MainWindow::onLoggedOut);

    // 开启定时器，更新状态栏时间
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        m_status_time->setText(
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        });
    timer->start(1000);  

    // 恢复上次关闭时的窗口位置
    QByteArray geom = ConfigManager::instance().loadWindowState("MainWindow");
    if (!geom.isEmpty()) {
        restoreGeometry(geom);
    }

	// 开启定时器，更新通信状态
    auto* comm_timer = new QTimer(this);
    connect(comm_timer, &QTimer::timeout, this, [this]() {
        auto all_status = CommManager::instance().getAllStatus();
        if (all_status.empty()) {
            m_status_comm->setText(QString::fromUtf8("通信: 仿真模式"));
            m_status_comm->setStyleSheet("color: #F39C12;");
        }
        else {
            bool all_ok = true;
            QString status_text;
            for (const auto& [name, status] : all_status) {
                if (!status_text.isEmpty()) status_text += " | ";
                switch (status) {
                case DeviceStatus::CONNECTED:
                    status_text += QString::fromStdString(name) + QString::fromUtf8(": 在线");
                    break;
                case DeviceStatus::DISCONNECTED:
                    status_text += QString::fromStdString(name) + QString::fromUtf8(": 离线");
                    all_ok = false;
                    break;
                case DeviceStatus::FAULT:
                    status_text += QString::fromStdString(name) + QString::fromUtf8(": 故障");
                    all_ok = false;
                    break;
                default:
                    status_text += QString::fromStdString(name) + ": ?";
                    all_ok = false;
                }
            }
            m_status_comm->setText(status_text);
            m_status_comm->setStyleSheet(
                all_ok ? "color: #2ECC71;" : "color: #E74C3C;");
        }
        });
    comm_timer->start(2000);

    LOG_INFO("MainWindow created");
}

// 设置UI
void MainWindow::setupUI()
{
    // 创建中央页面和布局：左侧导航 + 右侧页面
    auto* central = new QWidget(this);
    auto* h_layout = new QHBoxLayout(central);
    h_layout->setContentsMargins(0, 0, 0, 0);  
    h_layout->setSpacing(0);                     

    // 创建导航栏列表，并设置样式
    m_nav_list = new QListWidget();
    m_nav_list->setFixedWidth(160); 
    m_nav_list->setStyleSheet(
        "QListWidget { background-color: #B0B0B0; color: #2C3E50; "
        "font-size: 16px; font-weight: bold; border: none; }"
        "QListWidget::item { padding: 12px 15px; }"
        "QListWidget::item:selected { background-color: #4A90D9; color: white; }"
        "QListWidget::item:hover { background-color: #DDDDDD; }");

    // 创建右侧页面容器
    m_page_stack = new QStackedWidget();

    h_layout->addWidget(m_nav_list);
    h_layout->addWidget(m_page_stack, 1);  
    setCentralWidget(central);

    // 分别设置各组件UI
    setupMenuBar();
    setupNavigation();
    setupPages();
    setupStatusBar();

    // 连接导航栏切换信号和槽函数
    connect(m_nav_list, &QListWidget::currentRowChanged,
        this, &MainWindow::onNavigate);
}

// 设置菜单栏
void MainWindow::setupMenuBar()
{
    auto* menu_bar = menuBar();

    // 系统菜单
    auto* sys_menu = menu_bar->addMenu(QString::fromUtf8("系统(&S)"));
    m_action_login = new QAction(
        QString::fromUtf8("登录(&L)"), this);
    connect(m_action_login, &QAction::triggered,
        this, &MainWindow::onLogin);
    sys_menu->addAction(m_action_login);
    sys_menu->addSeparator();

    auto* exit_action = new QAction(
        QString::fromUtf8("退出(&X)"), this);
    exit_action->setShortcut(QKeySequence("Alt+F4"));  
    connect(exit_action, &QAction::triggered,
        this, &QMainWindow::close);
    sys_menu->addAction(exit_action);

    // 视图菜单
    auto* view_menu = menu_bar->addMenu(
        QString::fromUtf8("视图(&V)"));
    auto* fs_action = new QAction(
        QString::fromUtf8("全屏(&F)"), this);
    fs_action->setCheckable(true);             
    fs_action->setShortcut(QKeySequence("F11")); 
    connect(fs_action, &QAction::toggled, this, [this](bool checked) {
        if (checked) showFullScreen();
        else showNormal();
        });
    view_menu->addAction(fs_action);

    // 帮助菜单
    auto* help_menu = menu_bar->addMenu(
        QString::fromUtf8("帮助(&H)"));
    auto* about_action = new QAction(
        QString::fromUtf8("关于(&A)"), this);
    connect(about_action, &QAction::triggered, this, [this]() {
        QMessageBox::about(this,
            QString::fromUtf8("关于 UCS"),
            QString::fromUtf8("UCS 工业上位机监控系统 v1.0\n"
            "基于 Qt 6.5.3 + C++17\n"
                "精细化工反应釜过程控制"));
        });
    help_menu->addAction(about_action);
}

void MainWindow::setupNavigation()
{
	m_nav_list->addItem("监控页面");      // 0
	m_nav_list->addItem("数据分析");      // 1
	m_nav_list->addItem("报警管理");      // 2
	m_nav_list->addItem("配方管理");      // 3
	m_nav_list->addItem("生产报表");      // 4
	m_nav_list->addItem("系统日志");      // 5
	m_nav_list->setCurrentRow(0);
}

void MainWindow::setupPages()
{
	m_page_stack->addWidget(new MonitorWidget());          // 0
	m_page_stack->addWidget(new DataAnalysisWidget());     // 1
	m_page_stack->addWidget(new AlarmListWidget());        // 2
	m_page_stack->addWidget(new RecipeEditorWidget());     // 3
	m_page_stack->addWidget(new ReportWidget());           // 4
	m_page_stack->addWidget(new LogViewWidget());          // 5
}

// 设置状态栏UI
void MainWindow::setupStatusBar()
{
    auto* sb = statusBar();

    m_status_user = new QLabel(
        QString::fromUtf8("当前用户: 未登录"));
    sb->addWidget(m_status_user, 1); 

    m_status_comm = new QLabel(
        QString::fromUtf8("通信: 未连接"));
    m_status_comm->setStyleSheet("color: #E74C3C;");
    sb->addWidget(m_status_comm, 1);

    m_status_time = new QLabel();
    m_status_time->setAlignment(Qt::AlignRight);
    m_status_time->setStyleSheet("padding-right: 10px;");
    sb->addPermanentWidget(m_status_time);  
}

// 导航栏切换槽函数
void MainWindow::onNavigate(int row)
{
    if (row >= 0 && row < m_page_stack->count()) {
        m_page_stack->setCurrentIndex(row);
    }
}

// 登录槽函数
void MainWindow::onLogin()
{
    // 检查登录状态，已处于登录状态则登出
    if (UserManager::instance().isLoggedIn()) {
        UserManager::instance().logout();
        return;
    }

    LoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        LOG_INFO("Login dialog accepted");
    }
}

// 登录成功槽函数
void MainWindow::onLoginSucceeded(const QString& username)
{
    // 更新用户状态信息
    updateUserStatus();
}

// 登出槽函数
void MainWindow::onLoggedOut()
{
    // 更新用户状态信息
    updateUserStatus();
}

// 更新用户状态信息
void MainWindow::updateUserStatus()
{
    auto& um = UserManager::instance();

    if (um.isLoggedIn()) {
        auto& user = um.currentUser();
        m_status_user->setText(
            QString::fromUtf8("当前用户: %1 (%2)")
            .arg(QString::fromStdString(user.display_name))
            .arg(QString::fromStdString(roleName(user.role))));
        m_action_login->setText(
            QString::fromUtf8("登出(&L)"));
    }
    else {
        m_status_user->setText(
            QString::fromUtf8("当前用户: 未登录（仅查看）"));
        m_action_login->setText(
            QString::fromUtf8("登录(&L)"));
    }
}

// 重写窗口关闭事件，保存关闭前窗口状态信息
void MainWindow::closeEvent(QCloseEvent* event)
{
    ConfigManager::instance().saveWindowState("MainWindow", saveGeometry());
    LOG_INFO("MainWindow closing");
    event->accept();  
}

// ============================================================
// MainWindow.h — 主窗口声明
//
// 主窗口是整个上位机监控系统的顶层界面容器，采用经典的
// "工业上位机"布局：顶部菜单栏 + 左侧导航栏 + 右侧堆叠页面
// + 底部状态栏。整体结构示意如下：
//
//   ┌─────────────────────────────────────────────┐
//   │  系统(S)  视图(V)  帮助(H)        ← 菜单栏     │
//   ├──────────┬──────────────────────────────────┤
//   │ 监控页面 │                                  │
//   │ 数据分析 │         内容区域（堆叠页面）        │
//   │ 报警管理 │         QStackedWidget            │
//   │ 配方管理 │                                  │
//   │ 生产报表 │                                  │
//   │ 系统日志 │                                  │
//   ├──────────┴──────────────────────────────────┤
//   │ 当前用户: xxx | 通信: 在线 | 时间    ← 状态栏  │
//   └─────────────────────────────────────────────┘
//
// 左侧导航栏（QListWidget）与右侧页面容器（QStackedWidget）
// 通过索引一一对应，点击导航项即切换对应页面。
// ============================================================
#pragma once

#include <QMainWindow>        // 主窗口基类（自带菜单栏/状态栏/工具栏）
#include <QStackedWidget>      // 堆叠页面容器，用于右侧多页面切换
#include <QListWidget>         // 左侧导航栏列表
#include <QLabel>              // 状态栏文本标签

// 主窗口类：负责搭建整体框架、菜单栏、导航栏、状态栏，
// 并管理各功能页面的创建与切换，以及用户登录/登出的联动。
class MainWindow : public QMainWindow
{
    Q_OBJECT   // 使用信号槽所需的 Qt 元对象宏

public:
    // 构造函数：创建 UI、恢复窗口状态并建立信号槽连接
    explicit MainWindow(QWidget* parent = nullptr);
    // 析构函数：使用默认实现
    ~MainWindow() override = default;

protected:
    // 重写窗口关闭事件：在关闭前保存窗口位置与尺寸
    void closeEvent(QCloseEvent* event) override;

private slots:
    // 导航栏切换槽函数：根据选中行索引切换右侧页面
    void onNavigate(int row);
    // 登录槽函数：未登录时弹出登录对话框，已登录时执行登出
    void onLogin();
    // 登录成功槽函数：由 UserManager 的 loginSucceeded 信号触发
    void onLoginSucceeded(const QString& username);
    // 登出槽函数：由 UserManager 的 loggedOut 信号触发
    void onLoggedOut();

private:
    // 创建整体 UI（中央区域：左侧导航 + 右侧页面）
    void setupUI();
    // 创建菜单栏（系统/视图/帮助菜单）
    void setupMenuBar();
    // 创建左侧导航栏并填充功能项
    void setupNavigation();
    // 创建右侧各功能页面并加入堆叠容器
    void setupPages();
    // 创建底部状态栏（用户、通信、时间）
    void setupStatusBar();
    // 根据当前登录状态刷新用户信息显示
    void updateUserStatus();

    QListWidget* m_nav_list;         // 左侧导航栏列表
    QStackedWidget* m_page_stack;    // 右侧页面堆叠容器

    // 状态栏标签
    QLabel* m_status_user;  // 显示当前登录用户
    QLabel* m_status_comm;  // 显示通信/设备状态
    QLabel* m_status_time;  // 显示当前时间（每秒刷新）

    QAction* m_action_login;  // 菜单栏中的"登录/登出"动作
};

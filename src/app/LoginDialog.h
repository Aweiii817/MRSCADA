// ============================================================
// LoginDialog.h — 登录对话框声明
//
// 模态对话框：弹出后阻断主窗口交互，要求用户输入用户名和
// 密码完成登录，或点击"取消"放弃登录。
//
// 对话框内部不直接处理用户校验逻辑，而是调用
// UserManager::instance().login() 完成数据库校验，
// 根据返回结果决定接受（accept）还是继续停留在对话框。
// ============================================================
#pragma once

#include <QDialog>        // 对话框基类
#include <QLineEdit>      // 单行文本输入框（用户名、密码）
#include <QPushButton>    // 按钮（登录、取消）
#include <QLabel>         // 标签（标题、状态提示）

// 登录对话框类：负责收集用户输入并触发登录流程。
class LoginDialog : public QDialog
{
    Q_OBJECT   // 使用信号槽所需的 Qt 元对象宏

public:
    // 构造函数：创建并初始化界面
    explicit LoginDialog(QWidget* parent = nullptr);

private slots:
    // "登录"按钮点击槽函数：校验输入并调用 UserManager 登录
    void onLoginClicked();

private:
    // 创建并布局对话框内部的所有控件
    void setupUI();

    QLineEdit* m_username_edit;   // 用户名输入框
    QLineEdit* m_password_edit;   // 密码输入框（密文显示）
    QPushButton* m_login_btn;     // "登录"按钮
    QPushButton* m_cancel_btn;    // "取消"按钮
    QLabel* m_status_label;       // 状态标签，用于显示登录失败等提示信息
};

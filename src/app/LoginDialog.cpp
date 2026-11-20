// ============================================================
// LoginDialog.cpp — 登录对话框实现
// ============================================================
#include "LoginDialog.h"
#include "business/user/UserManager.h"

#include <QVBoxLayout>     
#include <QHBoxLayout>     
#include <QFormLayout>     

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)      
{
    setupUI();            
}

void LoginDialog::setupUI()
{
    // 设置标题栏文字
    setWindowTitle(QString::fromUtf8("用户登录 — UCS 上位机监控系统"));
    // 设置窗口固定大小
    setFixedSize(380, 240);    
    // 设置模态
    setModal(true);           

	// 创建主布局，并设置间距和边距
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(15);               
    main_layout->setContentsMargins(30, 20, 30, 20);  

	// 创建标题标签，设置字体大小、加粗和颜色
    auto* title = new QLabel(QString::fromUtf8("UCS 上位机监控系统"));
    title->setAlignment(Qt::AlignCenter);       
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #4A90D9;");
    main_layout->addWidget(title);

	// 创建表单布局，用于用户名和密码输入框
    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_username_edit = new QLineEdit();
    m_username_edit->setPlaceholderText(QString::fromUtf8("请输入用户名"));
    m_username_edit->setMinimumHeight(30);

    m_password_edit = new QLineEdit();
    m_password_edit->setPlaceholderText(QString::fromUtf8("请输入密码"));
    m_password_edit->setEchoMode(QLineEdit::Password);  
    m_password_edit->setMinimumHeight(30);

    form->addRow(QString::fromUtf8("用户名:"), m_username_edit);
    form->addRow(QString::fromUtf8("密  码:"), m_password_edit); 
    main_layout->addLayout(form);

    m_status_label = new QLabel();
    m_status_label->setStyleSheet("color: red;");   
    m_status_label->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(m_status_label);

    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(20);

    m_login_btn = new QPushButton(QString::fromUtf8("登 录"));
    m_login_btn->setDefault(true);   
    m_login_btn->setMinimumHeight(32);
    m_login_btn->setStyleSheet(
        "QPushButton { background-color: #4A90D9; color: white; "
        "border-radius: 4px; font-size: 14px; padding: 6px 24px; }"
        "QPushButton:hover { background-color: #357ABD; }");

    m_cancel_btn = new QPushButton(QString::fromUtf8("取 消"));
    m_cancel_btn->setMinimumHeight(32);

    btn_layout->addStretch();                
    btn_layout->addWidget(m_login_btn);
    btn_layout->addWidget(m_cancel_btn);
    btn_layout->addStretch();
    main_layout->addLayout(btn_layout);

    // 连接信号与槽
    connect(m_login_btn, &QPushButton::clicked,
        this, &LoginDialog::onLoginClicked);

    connect(m_cancel_btn, &QPushButton::clicked,
        this, &QDialog::reject);

    connect(m_password_edit, &QLineEdit::returnPressed,
        this, &LoginDialog::onLoginClicked);
}

// 登录按钮点击槽函数
void LoginDialog::onLoginClicked()
{
    // 取出输入信息
    std::string username = m_username_edit->text().toStdString();
    std::string password = m_password_edit->text().toStdString();

    // 检查用户名是否为空
    if (username.empty()) {
        m_status_label->setText(QString::fromUtf8("请输入用户名"));
        m_username_edit->setFocus();  
        return;
    }

    // 检查密码是否为空
    if (password.empty()) {
        m_status_label->setText(QString::fromUtf8("请输入密码"));
        m_password_edit->setFocus();
        return;
    }

    // 使用输出信息登录，检查登录是否成功
    if (UserManager::instance().login(username, password)) {
        accept();  
    }
    else {
        m_password_edit->clear();
        m_status_label->setText(QString::fromUtf8("用户名或密码错误，请重试"));
        m_password_edit->setFocus();
    }
}

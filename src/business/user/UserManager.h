// ============================================================
// UserManager.h — 用户管理器
//
// 管理"当前谁在使用系统"的会话状态。
// 整个系统只有一个"当前用户"。
//
// 用户账户数据持久化到 SQLite 数据库（users 表），
// 会话状态（当前用户、是否登录）仍保存在内存中。
// ============================================================
#pragma once

#include "User.h"
#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <string>

class UserManager : public QObject
{
    Q_OBJECT

public:
    // 获取用户管理器实例（单例）
    static UserManager& instance();

    // 初始化用户管理器：打开 SQLite 数据库、建表，并在首次运行时写入默认账户
    // db_path 为 SQLite 数据库文件路径，返回是否初始化成功
    bool initialize(const QString& db_path);

    // 用户登录：从数据库校验用户名与密码，成功则设置当前用户
    bool login(const std::string& username, const std::string& password);

    // 用户登出：重置当前用户为访客
    void logout();

    // 获取当前用户信息
    const UserInfo& currentUser() const;

    // 检查用户是否已登录
    bool isLoggedIn() const;

    // 检查当前用户是否具有指定权限
    bool hasPermission(Permission perm) const;

signals:
    // 登录成功信号（带用户名）
    void loginSucceeded(const QString& username);
    // 登录失败信号（带失败原因）
    void loginFailed(const QString& reason);
    // 登出信号
    void loggedOut();

private:
    UserManager();
    ~UserManager() override;

    // 用户数据结构（对应数据库 users 表的一行）
    struct StoredUser {
        std::string username;        // 用户名
        std::string password_hash;   // 密码哈希
        UserRole role;               // 角色
        bool is_active;              // 是否激活
    };

    // 计算密码的哈希值（简单哈希，生产环境应使用加盐强哈希）
    static std::string hashPassword(const std::string& password);

    // 创建 users 表（若不存在）
    bool createTable();

    // 首次运行时写入默认账户（admin / engineer / operator）
    void seedDefaultUsers();

    // 从数据库按用户名加载一条用户记录，成功返回 true
    bool loadUser(const std::string& username, StoredUser& out) const;

    QSqlDatabase m_db;            // SQLite 数据库连接
    UserInfo m_current_user;      // 当前用户信息（会话状态，内存）
    bool m_logged_in = false;     // 当前登录状态
    bool m_initialized = false;   // 数据库是否已初始化
};

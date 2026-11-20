// ============================================================
// UserManager.cpp — 用户管理器实现
// ============================================================
// 基于 SQLite 数据库实现用户账户的持久化存储：
//   initialize()     打开数据库、建表、首次运行写入默认账户
//   login()          从数据库校验用户名、激活状态、密码
//   logout()         重置当前用户为访客
//   以及 currentUser / isLoggedIn / hasPermission 等查询接口
// ============================================================
#include "UserManager.h"
#include "common/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDir>
#include <functional>

// 获取用户管理器实例（单例，局部静态变量保证线程安全）
UserManager& UserManager::instance()
{
    static UserManager inst;
    return inst;
}

// 构造函数，初始化当前用户为访客
UserManager::UserManager()
    : QObject(nullptr)
{
    m_current_user.username = "guest";
    m_current_user.display_name = "访客";
    m_current_user.role = UserRole::NONE;
    m_current_user.permissions = getDefaultPermissions(UserRole::NONE);
    m_current_user.is_active = true;
}

// 析构函数：关闭数据库连接
UserManager::~UserManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

// 计算密码的哈希值（简单哈希，生产环境应使用加盐强哈希）
std::string UserManager::hashPassword(const std::string& password)
{
    return std::to_string(std::hash<std::string>{}(password));
}

// 初始化用户管理器
// 步骤：确保目录存在 → 建立连接并打开 → 设置 PRAGMA → 建表 → 首次运行写入默认账户
bool UserManager::initialize(const QString& db_path)
{
    // 确保数据库文件所在目录存在（不存在则递归创建）
    QDir dir = QFileInfo(db_path).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 建立 SQLite 数据库连接（使用独立的连接名，避免与其他模块冲突）
    m_db = QSqlDatabase::addDatabase("QSQLITE", "user_connection");
    m_db.setDatabaseName(db_path);

    // 打开数据库，失败则记录错误并返回 false
    if (!m_db.open()) {
        LOG_ERROR("UserManager: failed to open database - {}",
            m_db.lastError().text().toStdString());
        return false;
    }

    // WAL：写操作先写入预写日志，提高并发读写性能
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA journal_mode=WAL");
    // NORMAL：普通同步级别，平衡性能与崩溃安全性
    pragma.exec("PRAGMA synchronous=NORMAL");

    // 创建 users 表（若不存在）
    if (!createTable()) {
        return false;
    }

    // 首次运行时写入默认账户
    seedDefaultUsers();

    m_initialized = true;
    LOG_INFO("UserManager: initialized at {}", db_path.toStdString());
    return true;
}

// 创建 users 表（若不存在）
// 表结构：username 主键 + 密码哈希 + 角色 + 是否激活
bool UserManager::createTable()
{
    QSqlQuery query(m_db);
    QString sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  username TEXT PRIMARY KEY,"
        "  password_hash TEXT NOT NULL,"
        "  role INTEGER NOT NULL,"
        "  is_active INTEGER NOT NULL DEFAULT 1"
        ")";

    if (!query.exec(sql)) {
        LOG_ERROR("UserManager: create table failed - {}",
            query.lastError().text().toStdString());
        return false;
    }
    return true;
}

// 首次运行时写入默认账户（admin / engineer / operator）
// 仅当 users 表为空时才写入，避免重复创建
void UserManager::seedDefaultUsers()
{
    // 查询表中是否已有用户，有则跳过写入
    QSqlQuery count_query(m_db);
    count_query.exec("SELECT COUNT(*) FROM users");
    if (count_query.next() && count_query.value(0).toInt() > 0) {
        return;
    }

    // 插入单个用户的辅助函数（参数绑定，防 SQL 注入）
    auto insertUser = [this](const std::string& username,
        const std::string& password, UserRole role) {
        QSqlQuery q(m_db);
        q.prepare("INSERT INTO users (username, password_hash, role, is_active) "
                  "VALUES (:u, :p, :r, 1)");
        q.bindValue(":u", QString::fromStdString(username));
        q.bindValue(":p", QString::fromStdString(hashPassword(password)));
        q.bindValue(":r", static_cast<int>(role));
        if (!q.exec()) {
            LOG_ERROR("UserManager: insert user '{}' failed - {}",
                username, q.lastError().text().toStdString());
        }
    };

    // 默认账户：admin / admin（管理员）
    insertUser("admin", "admin", UserRole::ADMIN);
    // 默认账户：engineer / engineer（工程师）
    insertUser("engineer", "engineer", UserRole::ENGINEER);
    // 默认账户：operator / operator（操作员）
    insertUser("operator", "operator", UserRole::OPERATOR);

    LOG_INFO("UserManager: seeded default users");
}

// 从数据库按用户名加载一条用户记录
bool UserManager::loadUser(const std::string& username, StoredUser& out) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT username, password_hash, role, is_active "
                  "FROM users WHERE username = :u");
    query.bindValue(":u", QString::fromStdString(username));

    if (!query.exec()) {
        LOG_ERROR("UserManager: query user '{}' failed - {}",
            username, query.lastError().text().toStdString());
        return false;
    }

    // 无匹配记录则返回 false
    if (!query.next()) {
        return false;
    }

    out.username = query.value(0).toString().toStdString();
    out.password_hash = query.value(1).toString().toStdString();
    out.role = static_cast<UserRole>(query.value(2).toInt());
    out.is_active = query.value(3).toInt() != 0;
    return true;
}

// 用户登录：从数据库校验用户名、激活状态与密码
bool UserManager::login(const std::string& username,
    const std::string& password)
{
    // 未初始化时拒绝登录
    if (!m_initialized) {
        LOG_ERROR("UserManager: login before initialize");
        emit loginFailed(QString::fromUtf8("系统未初始化"));
        return false;
    }

    // 从数据库加载用户，不存在则登录失败
    StoredUser user;
    if (!loadUser(username, user)) {
        LOG_WARN("User '{}' not found", username);
        emit loginFailed(QString::fromUtf8("用户名错误或不存在"));
        return false;
    }

    // 检查用户是否激活
    if (!user.is_active) {
        LOG_WARN("User '{}' is disabled", username);
        emit loginFailed(QString::fromUtf8("账户已被禁用"));
        return false;
    }

    // 验证密码：计算输入密码的哈希并与数据库中的哈希比较
    if (hashPassword(password) != user.password_hash) {
        LOG_WARN("User '{}' wrong password", username);
        emit loginFailed(QString::fromUtf8("密码错误"));
        return false;
    }

    // 登录成功，记录当前用户状态
    m_logged_in = true;
    m_current_user.username = user.username;
    m_current_user.display_name = user.username;
    m_current_user.role = user.role;
    m_current_user.permissions = getDefaultPermissions(user.role);
    m_current_user.is_active = true;

    LOG_INFO("User '{}' logged in as {}",
        username, roleName(user.role));

    emit loginSucceeded(QString::fromStdString(username));
    return true;
}

// 用户登出，重置当前用户为访客
void UserManager::logout()
{
    m_logged_in = false;
    m_current_user = { "guest",
                      "访客",
                      UserRole::NONE,
                      getDefaultPermissions(UserRole::NONE), true };
    LOG_INFO("User logged out");
    emit loggedOut();
}

// 获取当前用户信息
const UserInfo& UserManager::currentUser() const
{
    return m_current_user;
}

// 检查用户是否已登录
bool UserManager::isLoggedIn() const
{
    return m_logged_in;
}

// 检查当前用户是否具有指定权限
bool UserManager::hasPermission(Permission perm) const
{
    return m_current_user.hasPermission(perm);
}

// ============================================================
// User.h — 用户角色与权限定义
// ============================================================
// 定义用户相关的核心类型：
//   UserRole     用户角色枚举
//   Permission   用户权限枚举（位掩码，可组合）
//   UserInfo     用户信息结构体
//   以及默认权限查询、角色名称查询等辅助函数
// ============================================================
#pragma once

#include <string>
#include <cstdint>

// 用户角色枚举
// 权限由低到高：访客(无) < 操作员 < 工程师 < 管理员
enum class UserRole : uint8_t
{
    NONE = 0,       // 无权限（访客）
    OPERATOR = 1,   // 操作员
    ENGINEER = 2,   // 工程师
    ADMIN = 3       // 管理员
};

// 用户权限枚举，使用位掩码表示
// 每个权限占一个二进制位，可通过按位或组合出多个权限
enum class Permission : uint64_t
{
    VIEW_MONITOR = 1ULL << 0,   // 查看监控画面
    CONFIRM_ALARM = 1ULL << 1,  // 确认报警
    WRITE_PARAM = 1ULL << 2,    // 修改参数
    MANAGE_RECIPE = 1ULL << 3,  // 管理配方
    CONFIG_COMM = 1ULL << 4,    // 配置通信
    CONFIG_SYSTEM = 1ULL << 5,  // 配置系统
    MANAGE_USER = 1ULL << 6,    // 管理用户
    VIEW_HISTORY = 1ULL << 7,   // 查看历史数据
    EXPORT_DATA = 1ULL << 8,    // 导出数据
};

// 重载按位或运算符，用于组合多个权限
inline Permission operator|(Permission a, Permission b)
{
    return static_cast<Permission>(
        static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

// 重载按位与运算符，用于检查权限
inline Permission operator&(Permission a, Permission b)
{
    return static_cast<Permission>(
        static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

// 用户信息结构体
struct UserInfo
{
    std::string username;        // 用户名
    std::string display_name;    // 显示名称
    UserRole role;               // 用户角色
    Permission permissions;      // 用户权限
    bool is_active;              // 用户是否激活

    // 判断用户是否具有指定权限
    // 只有当权限位完整包含目标权限时才返回 true
    bool hasPermission(Permission perm) const
    {
        return (permissions & perm) == perm;
    }
};

// 获取指定角色的默认权限
// 角色等级越高，默认拥有越多的权限
inline Permission getDefaultPermissions(UserRole role)
{
    switch (role) {
    case UserRole::OPERATOR:
        // 操作员：查看监控、确认报警、查看历史
        return Permission::VIEW_MONITOR
            | Permission::CONFIRM_ALARM
            | Permission::VIEW_HISTORY;
    case UserRole::ENGINEER:
        // 工程师：操作员权限 + 修改参数、管理配方、导出数据
        return Permission::VIEW_MONITOR
            | Permission::CONFIRM_ALARM
            | Permission::VIEW_HISTORY
            | Permission::WRITE_PARAM
            | Permission::MANAGE_RECIPE
            | Permission::EXPORT_DATA;
    case UserRole::ADMIN:
        // 管理员：全部权限
        return Permission::VIEW_MONITOR
            | Permission::CONFIRM_ALARM
            | Permission::VIEW_HISTORY
            | Permission::WRITE_PARAM
            | Permission::MANAGE_RECIPE
            | Permission::CONFIG_COMM
            | Permission::CONFIG_SYSTEM
            | Permission::MANAGE_USER
            | Permission::EXPORT_DATA;
    default:
        // 访客：无任何权限
        return static_cast<Permission>(0);
    }
}

// 获取指定角色的名称（用于界面显示、日志输出等）
inline std::string roleName(UserRole role)
{
    switch (role) {
    case UserRole::OPERATOR: return "操作员";
    case UserRole::ENGINEER: return "工程师";
    case UserRole::ADMIN:    return "管理员";
    default:                 return "访客";
    }
}

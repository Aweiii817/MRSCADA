// ============================================================
// Recipe.h — 配方数据结构
//
// 配方 = 生产一个产品所需的所有工艺参数集合
// 用 JSON 持久化，可读取、可版本管理。
//
// 数据结构中字段用 std::optional 表示"可选参数"：
//   有的阶段控温度，有的控液位，
//   用 optional 区分"没有设置"和"设置为 0"。
// ============================================================
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

/// @brief 配方中的一个工艺阶段
struct RecipeStage
{
	std::string stage_name;             // 阶段名称，如 "进料"

	// 设定值（std::optional 为空 = 该阶段不控制此参数）
	std::optional<double> level_target;    // 目标液位（%）
	std::optional<double> temp_target;     // 目标温度（℃）
	std::optional<double> temp_rate;       // 升温/降温速率（℃/min）
	std::optional<double> agitator_speed;  // 搅拌转速（RPM）
	std::optional<double> valve_open;      // 相关阀门开度（0-1）

	uint32_t duration_sec = 0;        // 该阶段持续时间（秒）
	// 0 = 等到目标值达成，如 "液位到 80 就进入下一阶段"
};

/// @brief 工艺配方
struct Recipe
{
	std::string recipe_id;             // 配方编号，如 "RCP-001"
	std::string product_name;          // 产品名称
	std::string version = "v1.0";      // 版本号
	std::string author;                // 创建/修改人
	uint64_t create_time_ms = 0;       // 创建时间（毫秒）
	uint64_t modify_time_ms = 0;       // 最后修改时间（毫秒）
	std::vector<RecipeStage> stages;   // 工艺阶段列表
	std::string description;           // 备注说明
};

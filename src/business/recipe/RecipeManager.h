// ============================================================
// RecipeManager.h — 配方管理器
//
// 管理配方的增删改查与持久化。
// 配方存储为 JSON 文件，在 config/recipes/ 目录下，
// 每个配方一个 JSON 文件（便于单独管理和版本控制）。
// ============================================================
#pragma once

#include <QObject>
#include <vector>
#include <string>
#include "Recipe.h"

class RecipeManager : public QObject
{
	Q_OBJECT

public:
	// 单例访问接口
	static RecipeManager& instance();

	/// @brief 初始化：创建配方存储目录并加载已有配方
	/// @param recipe_dir 配方目录，如 "config/recipes"
	void initialize(const QString& recipe_dir);

	/// @brief 获取全部配方
	std::vector<Recipe> getAllRecipes() const;

	/// @brief 通过 ID 查找配方
	/// @return 找到返回 true
	bool findRecipe(const std::string& recipe_id, Recipe& out) const;

	/// @brief 添加配方
	/// @return true 成功，若 ID 已重复返回 false
	bool addRecipe(const Recipe& recipe);

	/// @brief 更新配方
	bool updateRecipe(const Recipe& recipe);

	/// @brief 删除配方
	bool deleteRecipe(const std::string& recipe_id);

	/// @brief 保存所有配方到文件
	void saveAll();

signals:
	/// @brief 配方列表发生变化（UI 刷新用）
	void recipesChanged();

private:
	RecipeManager();
	~RecipeManager() override = default;

	/// @brief 从 JSON 字符串解析单个配方
	Recipe recipeFromJson(const std::string& json_str) const;

	/// @brief 将配方序列化为 JSON 字符串
	std::string recipeToJson(const Recipe& recipe) const;

	/// @brief 从目录加载所有配方文件
	void loadAllRecipes();

	QString m_recipe_dir;           // 配方存储目录
	std::vector<Recipe> m_recipes;  // 内存中的配方列表
};

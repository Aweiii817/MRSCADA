// ============================================================
// RecipeManager.cpp — 配方管理器实现
// ============================================================
//
// 实现 RecipeManager：配方的增删改查、JSON 序列化/反序列化，
// 以及以独立 JSON 文件形式在 config/recipes/ 目录下的持久化。
// ============================================================

#include "RecipeManager.h"
#include "common/Logger.h"     // 日志工具
#include <nlohmann/json.hpp>   // JSON 解析库
#include <QDir>                // 目录操作
#include <QFile>               // 文件读写
#include <QFileInfo>           // 文件路径信息
#include <QDateTime>           // 时间戳

using Json = nlohmann::json;

// ------------------------------------------------------------
// instance() — 单例访问
// ------------------------------------------------------------
RecipeManager& RecipeManager::instance()
{
	static RecipeManager inst;
	return inst;
}

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
RecipeManager::RecipeManager()
	: QObject(nullptr)
{
}

// ------------------------------------------------------------
// initialize() — 初始化
// ------------------------------------------------------------
// 保存配方目录，确保目录存在，然后加载目录下所有配方文件。
// ------------------------------------------------------------
void RecipeManager::initialize(const QString& recipe_dir)
{
	m_recipe_dir = recipe_dir;

	// 确保目录存在（不存在则递归创建）
	QDir dir(recipe_dir);
	if (!dir.exists()) dir.mkpath(".");

	loadAllRecipes();
	LOG_INFO("RecipeManager: initialized, {} recipes loaded",
		m_recipes.size());
}

// ------------------------------------------------------------
// loadAllRecipes() — 加载所有配方文件
// ------------------------------------------------------------
// 遍历目录下所有 .json 文件，逐份解析为 Recipe 并加入内存列表。
// ------------------------------------------------------------
void RecipeManager::loadAllRecipes()
{
	m_recipes.clear();

	QDir dir(m_recipe_dir);
	// 读取目录下所有 .json 文件
	QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);

	for (const QString& file : files) {
		QFile f(dir.filePath(file));
		if (!f.open(QIODevice::ReadOnly)) continue;

		Recipe recipe = recipeFromJson(f.readAll().toStdString());
		f.close();

		// 仅当解析出有效 recipe_id 时加入列表
		if (!recipe.recipe_id.empty()) {
			m_recipes.push_back(recipe);
		}
	}
}

// ------------------------------------------------------------
// getAllRecipes() — 获取全部配方
// ------------------------------------------------------------
std::vector<Recipe> RecipeManager::getAllRecipes() const
{
	return m_recipes;
}

// ------------------------------------------------------------
// findRecipe() — 按 ID 查找配方
// ------------------------------------------------------------
// 线性遍历查找，找到则复制到 out 并返回 true。
// ------------------------------------------------------------
bool RecipeManager::findRecipe(const std::string& recipe_id, Recipe& out) const
{
	for (const auto& r : m_recipes) {
		if (r.recipe_id == recipe_id) {
			out = r;
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------
// addRecipe() — 添加配方
// ------------------------------------------------------------
// 若 ID 已存在则拒绝；否则加入列表、保存到文件并发信号。
// ------------------------------------------------------------
bool RecipeManager::addRecipe(const Recipe& recipe)
{
	// 检查 ID 是否重复
	for (const auto& r : m_recipes) {
		if (r.recipe_id == recipe.recipe_id) {
			LOG_WARN("RecipeManager: recipe '{}' already exists",
				recipe.recipe_id);
			return false;
		}
	}

	m_recipes.push_back(recipe);
	saveAll();
	emit recipesChanged();
	LOG_INFO("RecipeManager: added recipe '{}'", recipe.recipe_id);
	return true;
}

// ------------------------------------------------------------
// updateRecipe() — 更新配方
// ------------------------------------------------------------
// 找到相同 ID 的配方后整体替换，保存并发信号。
// ------------------------------------------------------------
bool RecipeManager::updateRecipe(const Recipe& recipe)
{
	for (auto& r : m_recipes) {
		if (r.recipe_id == recipe.recipe_id) {
			r = recipe;
			saveAll();
			emit recipesChanged();
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------
// deleteRecipe() — 删除配方
// ------------------------------------------------------------
bool RecipeManager::deleteRecipe(const std::string& recipe_id)
{
	for (auto it = m_recipes.begin(); it != m_recipes.end(); ++it) {
		if (it->recipe_id == recipe_id) {
			m_recipes.erase(it);
			saveAll();
			emit recipesChanged();
			LOG_INFO("RecipeManager: deleted recipe '{}'", recipe_id);
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------
// saveAll() — 保存所有配方到文件
// ------------------------------------------------------------
// 每个配方保存为独立 JSON 文件，文件名 = "<recipe_id>.json"。
// ------------------------------------------------------------
void RecipeManager::saveAll()
{
	for (const auto& recipe : m_recipes) {
		QString file_path = m_recipe_dir + "/"
			+ QString::fromStdString(recipe.recipe_id) + ".json";

		QFile f(file_path);
		if (!f.open(QIODevice::WriteOnly)) {
			LOG_ERROR("RecipeManager: cannot write {}", file_path.toStdString());
			continue;
		}

		f.write(recipeToJson(recipe).c_str());
		f.close();
	}
}

// ------------------------------------------------------------
// recipeFromJson() — 从 JSON 解析配方
// ------------------------------------------------------------
// 解析配方字段与阶段列表。
// std::optional 的处理：JSON 中只有存在该字段才赋值，
// 否则保持 nullopt（区分"未设置"与"设置为 0"）。
// ------------------------------------------------------------
Recipe RecipeManager::recipeFromJson(const std::string& json_str) const
{
	Recipe recipe;
	try {
		Json j = Json::parse(json_str);

		recipe.recipe_id = j.value("recipe_id", "");
		recipe.product_name = j.value("product_name", "");
		recipe.version = j.value("version", "v1.0");
		recipe.author = j.value("author", "");
		recipe.create_time_ms = j.value("create_time", 0);
		recipe.modify_time_ms = j.value("modify_time", 0);
		recipe.description = j.value("description", "");

		if (j.contains("stages") && j["stages"].is_array()) {
			for (const auto& s : j["stages"]) {
				RecipeStage stage;
				stage.stage_name = s.value("stage_name", "");

				// std::optional 的处理：
				// JSON 中只有存在该字段才赋值，否则保持 nullopt
				if (s.contains("level_target"))
					stage.level_target = s["level_target"].get<double>();
				if (s.contains("temp_target"))
					stage.temp_target = s["temp_target"].get<double>();
				if (s.contains("temp_rate"))
					stage.temp_rate = s["temp_rate"].get<double>();
				if (s.contains("agitator_speed"))
					stage.agitator_speed = s["agitator_speed"].get<double>();
				if (s.contains("valve_open"))
					stage.valve_open = s["valve_open"].get<double>();

				stage.duration_sec = s.value("duration_sec", 0);
				recipe.stages.push_back(stage);
			}
		}
	}
	catch (const std::exception& e) {
		LOG_ERROR("RecipeManager: parse failed - {}", e.what());
	}
	return recipe;
}

// ------------------------------------------------------------
// recipeToJson() — 将配方序列化为 JSON 字符串
// ------------------------------------------------------------
// std::optional 序列化：有值才写入对应字段。
// ------------------------------------------------------------
std::string RecipeManager::recipeToJson(const Recipe& recipe) const
{
	Json j;
	j["recipe_id"] = recipe.recipe_id;
	j["product_name"] = recipe.product_name;
	j["version"] = recipe.version;
	j["author"] = recipe.author;
	j["create_time"] = recipe.create_time_ms;
	j["modify_time"] = recipe.modify_time_ms;
	j["description"] = recipe.description;

	j["stages"] = Json::array();
	for (const auto& stage : recipe.stages) {
		Json s;
		s["stage_name"] = stage.stage_name;
		// std::optional 序列化：有值才写
		if (stage.level_target)    s["level_target"] = *stage.level_target;
		if (stage.temp_target)     s["temp_target"] = *stage.temp_target;
		if (stage.temp_rate)       s["temp_rate"] = *stage.temp_rate;
		if (stage.agitator_speed)  s["agitator_speed"] = *stage.agitator_speed;
		if (stage.valve_open)      s["valve_open"] = *stage.valve_open;
		s["duration_sec"] = stage.duration_sec;
		j["stages"].push_back(s);
	}

	return j.dump(4);   // 4 空格缩进，便于阅读
}

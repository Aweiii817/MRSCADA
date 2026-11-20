// ============================================================
// RecipeExecutor.cpp — 配方执行引擎实现
// ============================================================
//
// 实现 RecipeExecutor：将配方逐阶段"下发"到仿真引擎（或 PLC），
// 通过定时器周期检查当前阶段是否完成，完成后自动切换下一阶段。
// ============================================================

#include "RecipeExecutor.h"
#include "simulation/SimEngine.h"        // 仿真引擎
#include "simulation/SimReactorModel.h"  // 反应釜仿真模型
#include "core/rtdb/RealTimeDB.h"        // 实时数据库
#include "common/Logger.h"               // 日志工具

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
// 创建阶段检查定时器并连接槽函数。
// ------------------------------------------------------------
RecipeExecutor::RecipeExecutor(QObject* parent)
	: QObject(parent)
{
	m_timer = new QTimer(this);
	connect(m_timer, &QTimer::timeout, this, &RecipeExecutor::onTick);
}

// ------------------------------------------------------------
// 析构函数
// ------------------------------------------------------------
RecipeExecutor::~RecipeExecutor()
{
	stop();
}

// ------------------------------------------------------------
// bindEngine() — 绑定仿真引擎
// ------------------------------------------------------------
void RecipeExecutor::bindEngine(SimEngine* engine)
{
	m_engine = engine;
}

// ------------------------------------------------------------
// loadRecipe() — 加载配方
// ------------------------------------------------------------
// 校验配方至少有一个阶段，然后缓存配方并重置阶段索引。
// ------------------------------------------------------------
bool RecipeExecutor::loadRecipe(const Recipe& recipe)
{
	if (recipe.stages.empty()) {
		LOG_ERROR("RecipeExecutor: recipe has no stages");
		return false;
	}
	m_recipe = recipe;
	m_current_stage = -1;
	LOG_INFO("RecipeExecutor: loaded recipe '{}' ({} stages)",
		recipe.recipe_id, recipe.stages.size());
	return true;
}

// ------------------------------------------------------------
// start() — 开始执行
// ------------------------------------------------------------
// 从第一阶段开始，启动 200ms 周期检查。
// ------------------------------------------------------------
void RecipeExecutor::start()
{
	if (m_running || m_recipe.stages.empty()) return;

	m_running = true;
	switchToStage(0);   // 从第一阶段开始

	m_timer->start(200);   // 每 200ms 检查一次阶段状态
	LOG_INFO("RecipeExecutor: started recipe '{}'", m_recipe.recipe_id);
}

// ------------------------------------------------------------
// stop() — 停止执行
// ------------------------------------------------------------
void RecipeExecutor::stop()
{
	if (!m_running) return;
	m_timer->stop();
	m_running = false;
	LOG_INFO("RecipeExecutor: stopped");
}

// ------------------------------------------------------------
// isRunning() — 是否正在执行
// ------------------------------------------------------------
bool RecipeExecutor::isRunning() const
{
	return m_running;
}

// ------------------------------------------------------------
// currentStageIndex() — 当前阶段索引
// ------------------------------------------------------------
int RecipeExecutor::currentStageIndex() const
{
	return m_current_stage;
}

// ------------------------------------------------------------
// currentStageName() — 当前阶段名称
// ------------------------------------------------------------
// 未执行（索引 -1）或越界时返回"未执行"。
// ------------------------------------------------------------
QString RecipeExecutor::currentStageName() const
{
	if (m_current_stage < 0 ||
		m_current_stage >= static_cast<int>(m_recipe.stages.size())) {
		return QString::fromUtf8("未执行");
	}
	return QString::fromStdString(
		m_recipe.stages[m_current_stage].stage_name);
}

// ------------------------------------------------------------
// totalStages() — 总阶段数
// ------------------------------------------------------------
int RecipeExecutor::totalStages() const
{
	return static_cast<int>(m_recipe.stages.size());
}

// ------------------------------------------------------------
// switchToStage() — 切换到指定阶段
// ------------------------------------------------------------
// 将当前阶段索引设为 index，并把该阶段的设定值下发到仿真引擎：
//   1. 目标温度/液位写入模型参数；
//   2. 根据阶段名控制相应阀门（进料/加热/冷却/出料）；
//   3. 设置工艺阶段（ProcessStage）。
// 最后发出阶段切换信号。
// ------------------------------------------------------------
void RecipeExecutor::switchToStage(int index)
{
	if (index >= static_cast<int>(m_recipe.stages.size())) {
		return;
	}

	m_current_stage = index;
	const RecipeStage& stage = m_recipe.stages[index];

	// 把配方参数下发到仿真引擎
	if (m_engine) {
		auto& model = m_engine->model();
		auto& params = const_cast<ReactorParams&>(model.params());

		// 更新目标参数（仅当阶段设定了对应目标值）
		if (stage.temp_target)    params.setpoint_temp = *stage.temp_target;
		if (stage.level_target)   params.setpoint_level = *stage.level_target;

		// 设置阀门（根据阶段名判断开哪个阀）
		if (stage.valve_open) {
			const std::string& name = stage.stage_name;
			if (name == "进料") {
				model.setValve("HV-101", *stage.valve_open);
			}
			else if (name == "加热") {
				model.setValve("TV-101", *stage.valve_open);
			}
			else if (name == "冷却") {
				model.setValve("TV-102", *stage.valve_open);
			}
			else if (name == "出料") {
				model.setValve("HV-103", *stage.valve_open);
			}
		}

		// 设置工艺阶段
		const std::string& name = stage.stage_name;
		if (name == "进料")      model.setStage(ProcessStage::FEEDING);
		else if (name == "加热") model.setStage(ProcessStage::HEATING);
		else if (name == "恒温反应" || name == "恒温")
			model.setStage(ProcessStage::REACTING);
		else if (name == "冷却") model.setStage(ProcessStage::COOLING);
		else if (name == "出料") model.setStage(ProcessStage::DISCHARGING);
	}

	// 发出阶段切换信号
	emit stageChanged(index, currentStageName());

	LOG_INFO("RecipeExecutor: stage {} '{}' started",
		index, stage.stage_name);
}

// ------------------------------------------------------------
// onTick() — 定时器槽函数
// ------------------------------------------------------------
// 每个周期：判断当前阶段是否完成，完成则切换下一阶段；
// 全部完成则停止并发出完成信号；同时更新进度百分比。
// ------------------------------------------------------------
void RecipeExecutor::onTick()
{
	if (!m_running || !m_engine) return;

	// 判断当前阶段是否完成
	if (isStageComplete()) {
		// 切换到下一阶段
		int next = m_current_stage + 1;
		if (next >= static_cast<int>(m_recipe.stages.size())) {
			// 所有阶段完成
			m_running = false;
			m_timer->stop();
			m_current_stage = -1;
			emit progressUpdated(100);
			emit recipeFinished();
			LOG_INFO("RecipeExecutor: recipe '{}' finished",
				m_recipe.recipe_id);
			return;
		}
		switchToStage(next);
	}

	// 更新进度（已完成阶段数 / 总阶段数）
	int percent = static_cast<int>(
		(m_current_stage + 1.0) / m_recipe.stages.size() * 100);
	emit progressUpdated(percent);
}

// ------------------------------------------------------------
// isStageComplete() — 判断当前阶段是否完成
// ------------------------------------------------------------
// 分两类判定：
//   1. 时间型阶段（duration_sec > 0）：持续时间达到即完成；
//   2. 目标型阶段：达到对应目标值才完成
//      （进料/出料按液位，加热/冷却按温度，并留有一定余量）。
// ------------------------------------------------------------
bool RecipeExecutor::isStageComplete() const
{
	if (!m_engine || m_current_stage < 0 ||
		m_current_stage >= static_cast<int>(m_recipe.stages.size())) {
		return false;
	}

	const RecipeStage& stage = m_recipe.stages[m_current_stage];
	const auto& state = m_engine->model().getState();

	// 时间型阶段：持续时间到了就算完成
	if (stage.duration_sec > 0) {
		return m_engine->model().stageElapsedTime() >= stage.duration_sec;
	}

	// 目标型阶段：达到目标值才算完成
	const std::string& name = stage.stage_name;
	if (name == "进料") {
		return stage.level_target &&
			state.level >= *stage.level_target * 0.98;
	}
	if (name == "加热") {
		return stage.temp_target &&
			state.temperature >= *stage.temp_target * 0.97;
	}
	if (name == "冷却") {
		return stage.temp_target &&
			state.temperature <= *stage.temp_target;
	}
	if (name == "出料") {
		return stage.level_target &&
			state.level <= *stage.level_target;
	}

	return false;
}

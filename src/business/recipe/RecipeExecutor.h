// ============================================================
// RecipeExecutor.h — 配方执行引擎
//
// 把配方"翻译"成对仿真引擎（或 PLC）的控制指令。
// 维护当前阶段索引、计时，阶段完成后自动切换到下一阶段，
// 直至全部阶段执行完毕发出完成信号。
// ============================================================
#pragma once

#include <QObject>
#include <QTimer>
#include "Recipe.h"

class SimEngine;

class RecipeExecutor : public QObject
{
	Q_OBJECT

public:
	explicit RecipeExecutor(QObject* parent = nullptr);
	~RecipeExecutor() override;

	/// @brief 绑定仿真引擎（执行时下发参数）
	void bindEngine(SimEngine* engine);

	/// @brief 加载配方（准备执行）
	bool loadRecipe(const Recipe& recipe);

	/// @brief 开始执行配方（从第一阶段开始）
	void start();

	/// @brief 停止执行
	void stop();

	/// @brief 是否正在执行
	bool isRunning() const;

	/// @brief 获取当前阶段索引（-1 表示未执行）
	int currentStageIndex() const;

	/// @brief 获取当前阶段名称
	QString currentStageName() const;

	/// @brief 获取总阶段数
	int totalStages() const;

signals:
	/// @brief 阶段切换
	void stageChanged(int index, const QString& stage_name);

	/// @brief 配方执行完成
	void recipeFinished();

	/// @brief 配方执行进度更新（百分比）
	void progressUpdated(int percent);

private slots:
	/// @brief 定时器槽函数：检查阶段状态
	void onTick();

private:
	/// @brief 切换到指定阶段（下发设定值）
	void switchToStage(int index);

	/// @brief 判断当前阶段是否完成
	bool isStageComplete() const;

	SimEngine* m_engine = nullptr;   // 被控制的仿真引擎
	Recipe m_recipe;                 // 当前配方
	int m_current_stage = -1;        // 当前阶段索引
	bool m_running = false;          // 是否正在执行
	QTimer* m_timer = nullptr;       // 阶段检查定时器
};

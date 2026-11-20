// ============================================================
// RecipeEditorWidget.h — 配方管理页面
// ============================================================
//
// RecipeEditorWidget 是“配方管理”页面，继承自 QWidget。用于展示
// 全部配方列表，并支持新建、删除配方。配方数据来自 RecipeManager
// 单例，表格展示配方编号、产品名称、版本、阶段数、作者与备注。
// ============================================================

#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;

class RecipeEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecipeEditorWidget(QWidget* parent = nullptr);

private slots:
    // 刷新配方列表
    void refreshList();

    // 新建配方
    void onCreateRecipe();

    // 删除选中配方
    void onDeleteRecipe();

private:
    void setupUI();

    QTableWidget* m_table = nullptr;
    QPushButton* m_create_btn = nullptr;
    QPushButton* m_delete_btn = nullptr;
    QPushButton* m_refresh_btn = nullptr;
};

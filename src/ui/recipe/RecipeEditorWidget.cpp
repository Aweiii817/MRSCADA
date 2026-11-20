// ============================================================
// RecipeEditorWidget.cpp — 配方管理页面实现
// ============================================================
//
// 实现 RecipeEditorWidget：以表格展示全部配方，并支持新建与删除。
// 数据来源于 RecipeManager 单例，配方变更信号会触发列表自动刷新。
// ============================================================

#include "RecipeEditorWidget.h"
#include "business/recipe/RecipeManager.h"
#include "business/recipe/Recipe.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateTime>

RecipeEditorWidget::RecipeEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    // 配方列表变化时自动刷新
    connect(&RecipeManager::instance(), &RecipeManager::recipesChanged,
        this, &RecipeEditorWidget::refreshList);

    refreshList();
}

void RecipeEditorWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto* title = new QLabel(QString::fromUtf8("配方管理"));
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(title);

    // 工具栏：新建 / 删除 / 刷新
    auto* toolbar = new QHBoxLayout();
    m_create_btn = new QPushButton(QString::fromUtf8("新建配方"));
    m_delete_btn = new QPushButton(QString::fromUtf8("删除配方"));
    m_refresh_btn = new QPushButton(QString::fromUtf8("刷新"));
    connect(m_create_btn, &QPushButton::clicked,
        this, &RecipeEditorWidget::onCreateRecipe);
    connect(m_delete_btn, &QPushButton::clicked,
        this, &RecipeEditorWidget::onDeleteRecipe);
    connect(m_refresh_btn, &QPushButton::clicked,
        this, &RecipeEditorWidget::refreshList);
    toolbar->addWidget(m_create_btn);
    toolbar->addWidget(m_delete_btn);
    toolbar->addWidget(m_refresh_btn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    // 配方列表表格
    m_table = new QTableWidget();
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QString::fromUtf8("配方编号"),
        QString::fromUtf8("产品名称"),
        QString::fromUtf8("版本"),
        QString::fromUtf8("阶段数"),
        QString::fromUtf8("作者"),
        QString::fromUtf8("备注")
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    layout->addWidget(m_table, 1);
}

void RecipeEditorWidget::refreshList()
{
    m_table->setRowCount(0);

    auto recipes = RecipeManager::instance().getAllRecipes();
    m_table->setRowCount(static_cast<int>(recipes.size()));

    for (int i = 0; i < static_cast<int>(recipes.size()); ++i) {
        const Recipe& r = recipes[i];
        m_table->setItem(i, 0,
            new QTableWidgetItem(QString::fromStdString(r.recipe_id)));
        m_table->setItem(i, 1,
            new QTableWidgetItem(QString::fromStdString(r.product_name)));
        m_table->setItem(i, 2,
            new QTableWidgetItem(QString::fromStdString(r.version)));
        m_table->setItem(i, 3,
            new QTableWidgetItem(QString::number(r.stages.size())));
        m_table->setItem(i, 4,
            new QTableWidgetItem(QString::fromStdString(r.author)));
        m_table->setItem(i, 5,
            new QTableWidgetItem(QString::fromStdString(r.description)));
    }
}

void RecipeEditorWidget::onCreateRecipe()
{
    // 用对话框收集配方基本信息
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("新建配方"));

    auto* form = new QFormLayout(&dlg);
    auto* id_edit = new QLineEdit();
    auto* name_edit = new QLineEdit();
    auto* version_edit = new QLineEdit(QString::fromUtf8("v1.0"));
    auto* author_edit = new QLineEdit();
    auto* desc_edit = new QLineEdit();

    form->addRow(QString::fromUtf8("配方编号:"), id_edit);
    form->addRow(QString::fromUtf8("产品名称:"), name_edit);
    form->addRow(QString::fromUtf8("版本:"), version_edit);
    form->addRow(QString::fromUtf8("作者:"), author_edit);
    form->addRow(QString::fromUtf8("备注:"), desc_edit);

    auto* buttons = new QHBoxLayout();
    auto* ok_btn = new QPushButton(QString::fromUtf8("确定"));
    auto* cancel_btn = new QPushButton(QString::fromUtf8("取消"));
    buttons->addStretch();
    buttons->addWidget(ok_btn);
    buttons->addWidget(cancel_btn);
    form->addRow(buttons);

    connect(ok_btn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel_btn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    QString id = id_edit->text().trimmed();
    if (id.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
            QString::fromUtf8("配方编号不能为空"));
        return;
    }

    Recipe r;
    r.recipe_id = id.toStdString();
    r.product_name = name_edit->text().trimmed().toStdString();
    r.version = version_edit->text().trimmed().toStdString();
    r.author = author_edit->text().trimmed().toStdString();
    r.description = desc_edit->text().trimmed().toStdString();
    r.create_time_ms = static_cast<uint64_t>(
        QDateTime::currentMSecsSinceEpoch());
    r.modify_time_ms = r.create_time_ms;

    if (!RecipeManager::instance().addRecipe(r)) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
            QString::fromUtf8("配方编号已存在，添加失败"));
    }
}

void RecipeEditorWidget::onDeleteRecipe()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QString::fromUtf8("提示"),
            QString::fromUtf8("请先选择一个配方"));
        return;
    }

    auto* item = m_table->item(row, 0);
    if (!item) return;

    QString id = item->text();
    auto ret = QMessageBox::question(this, QString::fromUtf8("确认删除"),
        QString::fromUtf8("确定要删除配方 %1 吗？").arg(id),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    RecipeManager::instance().deleteRecipe(id.toStdString());
}

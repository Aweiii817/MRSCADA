// ============================================================
// ReportWidget.cpp — 报表页面实现
// ============================================================
//
// 实现 ReportWidget：按日期生成生产日报 HTML，预览并支持导出
// PDF / HTML 文件。报表内容由 ReportGenerator 生成。
// ============================================================

#include "ReportWidget.h"
#include "business/recipe/ReportGenerator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

ReportWidget::ReportWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    onGenerate();
}

void ReportWidget::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto* title = new QLabel(QString::fromUtf8("报表"));
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(title);

    // 工具栏：日期选择 + 生成 / 导出按钮
    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel(QString::fromUtf8("报表日期:")));

    m_date_edit = new QDateEdit(QDate::currentDate());
    m_date_edit->setCalendarPopup(true);
    m_date_edit->setDisplayFormat("yyyy-MM-dd");
    toolbar->addWidget(m_date_edit);

    m_generate_btn = new QPushButton(QString::fromUtf8("生成报表"));
    m_export_pdf_btn = new QPushButton(QString::fromUtf8("导出 PDF"));
    m_export_html_btn = new QPushButton(QString::fromUtf8("导出 HTML"));
    connect(m_generate_btn, &QPushButton::clicked,
        this, &ReportWidget::onGenerate);
    connect(m_export_pdf_btn, &QPushButton::clicked,
        this, &ReportWidget::onExportPdf);
    connect(m_export_html_btn, &QPushButton::clicked,
        this, &ReportWidget::onExportHtml);
    toolbar->addWidget(m_generate_btn);
    toolbar->addWidget(m_export_pdf_btn);
    toolbar->addWidget(m_export_html_btn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    // HTML 预览区
    m_preview = new QTextBrowser();
    layout->addWidget(m_preview, 1);
}

void ReportWidget::onGenerate()
{
    m_current_html = ReportGenerator::generateDailyReportHtml(
        m_date_edit->date());
    m_preview->setHtml(m_current_html);
}

void ReportWidget::onExportPdf()
{
    QString path = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("导出 PDF"),
        QString::fromUtf8("日报.pdf"),
        QString::fromUtf8("PDF 文件 (*.pdf)"));
    if (path.isEmpty()) return;

    if (!ReportGenerator::exportToPdf(m_current_html, path)) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
            QString::fromUtf8("PDF 导出失败"));
    }
}

void ReportWidget::onExportHtml()
{
    QString path = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("导出 HTML"),
        QString::fromUtf8("日报.html"),
        QString::fromUtf8("HTML 文件 (*.html)"));
    if (path.isEmpty()) return;

    if (!ReportGenerator::saveToHtml(m_current_html, path)) {
        QMessageBox::warning(this, QString::fromUtf8("提示"),
            QString::fromUtf8("HTML 导出失败"));
    }
}

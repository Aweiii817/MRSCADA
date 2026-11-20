// ============================================================
// ReportWidget.h — 报表页面
// ============================================================
//
// ReportWidget 是“报表”页面，继承自 QWidget。用于按日期生成生产
// 日报，以 HTML 形式预览，并支持导出为 PDF 或 HTML 文件。
// ============================================================

#pragma once

#include <QWidget>
#include <QDate>

class QDateEdit;
class QTextBrowser;
class QPushButton;

class ReportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ReportWidget(QWidget* parent = nullptr);

private slots:
    // 生成报表并刷新预览
    void onGenerate();

    // 导出为 PDF 文件
    void onExportPdf();

    // 导出为 HTML 文件
    void onExportHtml();

private:
    void setupUI();

    QDateEdit* m_date_edit = nullptr;
    QTextBrowser* m_preview = nullptr;
    QPushButton* m_generate_btn = nullptr;
    QPushButton* m_export_pdf_btn = nullptr;
    QPushButton* m_export_html_btn = nullptr;

    QString m_current_html;   // 当前生成的 HTML 内容
};

// ============================================================
// LogViewWidget.cpp — 日志查看界面实现
// ============================================================
#include "LogViewWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QFont>
#include <QColor>

LogViewWidget::LogViewWidget(QWidget* parent)
	: QWidget(parent)
{
	setupUI();

	m_refresh_timer = new QTimer(this);
	connect(m_refresh_timer, &QTimer::timeout,
		this, &LogViewWidget::reloadLogs);
	m_refresh_timer->start(3000);   // 每 3 秒自动刷新
}

void LogViewWidget::setupUI()
{
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(8);

	// ---- 工具栏 ----
	auto* toolbar = new QHBoxLayout();
	toolbar->addWidget(new QLabel("级别过滤:"));

	m_level_filter = new QComboBox();
	m_level_filter->addItem("全部");
	m_level_filter->addItem("INFO");
	m_level_filter->addItem("WARNING");
	m_level_filter->addItem("ERROR");
	connect(m_level_filter, &QComboBox::currentTextChanged,
		this, &LogViewWidget::reloadLogs);
	toolbar->addWidget(m_level_filter);

	m_reload_btn = new QPushButton("刷新");
	connect(m_reload_btn, &QPushButton::clicked,
		this, &LogViewWidget::reloadLogs);
	toolbar->addWidget(m_reload_btn);

	m_clear_btn = new QPushButton("清空");
	connect(m_clear_btn, &QPushButton::clicked,
		this, &LogViewWidget::clearLogs);
	toolbar->addWidget(m_clear_btn);

	toolbar->addStretch();
	layout->addLayout(toolbar);

	// ---- 日志列表 ----
	m_log_list = new QListWidget();
	m_log_list->setFont(QFont("Consolas", 9));   // 等宽字体，对齐好看
	layout->addWidget(m_log_list, 1);
}

void LogViewWidget::reloadLogs()
{
	m_log_list->clear();

	QFile file(m_log_path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		m_log_list->addItem("[无法打开日志文件: " + m_log_path + "]");
		return;
	}

	QString filter = m_level_filter->currentText();

	// 读取所有行（日志文件不会太大，直接全读）
	// 只取最后 500 行，避免界面卡顿
	QStringList all_lines;
	while (!file.atEnd()) {
		all_lines << QString::fromUtf8(file.readLine());
	}
	file.close();

	// 倒序取最后 500 行
	int start = qMax(0, all_lines.size() - 500);
	for (int i = start; i < all_lines.size(); ++i) {
		QString line = all_lines[i].trimmed();
		if (line.isEmpty()) continue;

		// 级别过滤
		if (filter != "全部" && !line.contains(filter)) {
			continue;
		}

		auto* item = new QListWidgetItem(line);

		// 🎯 按级别着色
		if (line.contains("error", Qt::CaseInsensitive) ||
			line.contains("critical", Qt::CaseInsensitive)) {
			item->setForeground(QColor("#E74C3C"));   // 红
		}
		else if (line.contains("warn", Qt::CaseInsensitive)) {
			item->setForeground(QColor("#F39C12"));   // 黄
		}
		else if (line.contains("debug", Qt::CaseInsensitive)) {
			item->setForeground(QColor("#7F8C8D"));   // 灰
		}
		else {
			item->setForeground(QColor("#2ECC71"));   // 绿（info）
		}

		m_log_list->addItem(item);
	}

	// 滚动到底部（最新日志）
	m_log_list->scrollToBottom();
}

void LogViewWidget::clearLogs()
{
	m_log_list->clear();
}
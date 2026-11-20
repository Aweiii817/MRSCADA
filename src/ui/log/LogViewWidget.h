// ============================================================
// LogViewWidget.h — 系统日志查看页面
//
// 读取日志文件并展示在列表中，支持级别过滤和自动刷新。
// ============================================================
#pragma once

#include <QWidget>
#include <QTimer>

class QListWidget;
class QComboBox;
class QPushButton;

class LogViewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit LogViewWidget(QWidget* parent = nullptr);

private slots:
	/// @brief 重新加载日志
	void reloadLogs();

	/// @brief 清空显示
	void clearLogs();

private:
	void setupUI();

	QListWidget* m_log_list;
	QComboBox* m_level_filter;     // 级别过滤器
	QPushButton* m_reload_btn;
	QPushButton* m_clear_btn;
	QTimer* m_refresh_timer;

	QString m_log_path = "logs/ucs.log";   // 日志文件路径
};

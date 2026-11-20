// ============================================================
// ReportGenerator.cpp — 报表生成实现
// ============================================================
//
// 实现 ReportGenerator：生成生产日报 HTML（含生产数据汇总表、
// 报警记录表），并支持导出为 PDF 或 HTML 文件。
// ============================================================

#include "ReportGenerator.h"
#include "core/rtdb/RealTimeDB.h"      // 实时数据库（取测点值）
#include "core/alarm/AlarmEngine.h"    // 报警引擎（取报警记录）
#include "core/history/HistoryDB.h"    // 历史数据库
#include "common/Logger.h"             // 日志工具

#include <QTextDocument>   // 加载 HTML 并排版（用于打印）
#include <QPrinter>        // 打印输出（生成 PDF）
#include <QFile>           // 文件写入

// ------------------------------------------------------------
// generateDailyReportHtml() — 生成日报 HTML
// ------------------------------------------------------------
// 按「页头 → 生产数据汇总表 → 报警记录表 → 页脚」顺序拼装 HTML。
// ------------------------------------------------------------
QString ReportGenerator::generateDailyReportHtml(const QDate& date)
{
	QString html;
	html += "<html><head><meta charset='utf-8'></head><body>";
	html += buildHeader(date);            // 页头（标题 + 日期时间）
	html += buildProductionTable();       // 生产数据汇总表
	html += buildAlarmTable();            // 报警记录表
	html += buildFooter();                // 页脚
	html += "</body></html>";
	return html;
}

// ------------------------------------------------------------
// buildHeader() — 生成报表头
// ------------------------------------------------------------
// 标题 + 报表日期 + 生成时间。
// ------------------------------------------------------------
QString ReportGenerator::buildHeader(const QDate& date)
{
	return QString(
		"<h1 style='text-align:center;'>UCS 生产日报</h1>"
		"<p style='text-align:center;'>日期：%1    "
		"生成时间：%2</p>"
		"<hr>"
	).arg(date.toString("yyyy-MM-dd"))
		.arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
}

// ------------------------------------------------------------
// buildProductionTable() — 生成生产数据汇总表
// ------------------------------------------------------------
// 从 RealTimeDB 取所有测点最新值，逐条生成表格行。
// 质量 >= UNCERTAIN 视为正常，否则视为故障。
// ------------------------------------------------------------
QString ReportGenerator::buildProductionTable()
{
	// 从 RealTimeDB 取当前所有测点值
	auto values = RealTimeDB::instance().getAllLatestValues();

	QString html = "<h2>一、生产数据汇总</h2>";
	html += "<table border='1' cellspacing='0' cellpadding='5'>";
	html += "<tr style='background:#e0e0e0;'>"
		"<th>位号</th><th>描述</th><th>数值</th><th>单位</th>"
		"<th>质量</th><th>时间</th></tr>";

	for (const auto& pt : values) {
		QString quality = (pt.quality >= Quality::UNCERTAIN) ? "正常" : "故障";
		QDateTime ts = QDateTime::fromMSecsSinceEpoch(
			static_cast<qint64>(pt.timestamp_ms));

		html += QString(
			"<tr>"
			"<td>%1</td><td>%2</td><td>%3</td><td>%4</td>"
			"<td>%5</td><td>%6</td>"
			"</tr>")
			.arg(QString::fromStdString(pt.tag_name))
			.arg(QString::fromStdString(pt.description))
			.arg(pt.scaled_value, 0, 'f', 1)
			.arg(QString::fromStdString(pt.unit))
			.arg(quality)
			.arg(ts.toString("HH:mm:ss"));
	}

	html += "</table>";
	return html;
}

// ------------------------------------------------------------
// buildAlarmTable() — 生成报警记录表
// ------------------------------------------------------------
// 从 AlarmEngine 取全部报警事件，逐条生成表格行。
// 无报警时输出提示信息。
// ------------------------------------------------------------
QString ReportGenerator::buildAlarmTable()
{
	auto events = AlarmEngine::instance().getAllEvents();

	QString html = "<h2>二、报警记录</h2>";
	if (events.empty()) {
		html += "<p>当日无报警记录。</p>";
		return html;
	}

	html += "<table border='1' cellspacing='0' cellpadding='5'>";
	html += "<tr style='background:#e0e0e0;'>"
		"<th>时间</th><th>报警描述</th><th>位号</th>"
		"<th>触发值</th><th>状态</th></tr>";

	for (const auto& ev : events) {
		QDateTime ts = QDateTime::fromMSecsSinceEpoch(
			static_cast<qint64>(ev.trigger_time_ms));

		// 将报警状态枚举转换为中文文本
		QString state;
		switch (ev.state) {
		case AlarmState::ACTIVE:        state = "未确认"; break;
		case AlarmState::ACKNOWLEDGED:  state = "已确认"; break;
		case AlarmState::CLEARED_UNACK: state = "恢复待确认"; break;
		case AlarmState::INACTIVE:      state = "已关闭"; break;
		}

		html += QString(
			"<tr><td>%1</td><td>%2</td><td>%3</td>"
			"<td>%4</td><td>%5</td></tr>")
			.arg(ts.toString("HH:mm:ss"))
			.arg(QString::fromStdString(ev.message))
			.arg(QString::fromStdString(ev.tag_name))
			.arg(ev.trigger_value, 0, 'f', 1)
			.arg(state);
	}

	html += "</table>";
	return html;
}

// ------------------------------------------------------------
// buildFooter() — 生成报表尾注
// ------------------------------------------------------------
QString ReportGenerator::buildFooter()
{
	return "<hr><p style='text-align:center;font-size:12px;'>"
		"本报表由 UCS 上位机系统自动生成</p>";
}

// ------------------------------------------------------------
// exportToPdf() — 将 HTML 导出为 PDF
// ------------------------------------------------------------
// 用 QTextDocument 加载 HTML 并排版，再通过 QPrinter 打印为 PDF。
// ------------------------------------------------------------
bool ReportGenerator::exportToPdf(const QString& html,
	const QString& file_path)
{
	// QTextDocument 加载 HTML 并排版
	QTextDocument doc;
	doc.setHtml(html);

	// QPrinter 输出到 PDF
	QPrinter printer(QPrinter::HighResolution);
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(file_path);

	// 把文档"打印"到 PDF
	doc.print(&printer);

	LOG_INFO("ReportGenerator: exported PDF to {}",
		file_path.toStdString());
	return true;
}

// ------------------------------------------------------------
// saveToHtml() — 将 HTML 导出为 .html 文件
// ------------------------------------------------------------
bool ReportGenerator::saveToHtml(const QString& html,
	const QString& file_path)
{
	QFile file(file_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}
	file.write(html.toUtf8());   // 以 UTF-8 写入文件
	file.close();
	return true;
}

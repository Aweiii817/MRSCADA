// ============================================================
// ReportGenerator.h — 报表生成器
//
// 生成生产日报/班报的报表，输出两种格式：
//   - HTML 字符串，可保存为 .html 文件；
//   - PDF 文件（通过 QPrinter 打印）。
// ============================================================
#pragma once

#include <QString>
#include <QDateTime>

class ReportGenerator
{
public:
	/// @brief 生成日报 HTML
	/// @param date 报表日期
	/// @return HTML 字符串
	static QString generateDailyReportHtml(const QDate& date);

	/// @brief 将 HTML 报表导出为 PDF 文件
	/// @param html HTML 内容
	/// @param file_path 目标 PDF 路径
	/// @return true 成功
	static bool exportToPdf(const QString& html, const QString& file_path);

	/// @brief 将 HTML 报表导出为 .html 文件
	static bool saveToHtml(const QString& html, const QString& file_path);

private:
	/// @brief 生成报表头（标题 + 时间）
	static QString buildHeader(const QDate& date);

	/// @brief 生成生产数据汇总表
	static QString buildProductionTable();

	/// @brief 生成报警统计表
	static QString buildAlarmTable();

	/// @brief 生成报表尾注
	static QString buildFooter();
};

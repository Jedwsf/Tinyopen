// =============================================================================
// core/logger.cpp — 后台记录器实现
// =============================================================================
//
// 【存储格式】
//   文件：程序目录 / tinylog.txt（UTF-8）
//   每行：[yyyy-MM-dd HH:mm:ss] [分类] 正文
//   分类：信息 / 成功 / 失败 / 记事本
//
// 【函数一览】
//   Logger()          构造即清空旧记录（实现「只记本次运行」）
//   kFilePath()       返回「程序目录/tinylog.txt」
//   append()          追加一行（超上限丢最早一行）并立即落盘
//   info()            记「信息」
//   success()         记「成功」
//   notepad()         记「记事本」（无默认程序、退回记事本打开）
//   fail()            记「失败」，可附「原因：…」
//   setRaw()          用一组现成记录覆盖内存内容（预留接口，目前未使用）
//   toDisplayText()   拼成多行文本供 LogDialog 展示；空时返回「（暂无记录）」
//   clear()           清空并落盘
//   save()            整体重写文件（UTF-8）
// =============================================================================

#include "logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

// 构造：清空上一次的记录文件，实现「只记本次运行」
Logger::Logger(QObject *parent)
    : QObject(parent)
{
    // 只记录本次程序的运行情况：启动时清空上一次的记录
    m_lines.clear();
    save();
}

// 日志文件完整路径 = 程序所在目录 + /tinylog.txt
const QString Logger::kFilePath()
{
    // 日志放在程序所在文件夹（便携：整个文件夹拷走即可）
    return QCoreApplication::applicationDirPath() + "/tinylog.txt";
}

// 追加一行记录：[时间] [分类] 正文，超过上限则丢弃最早的行
void Logger::append(const QString &tag, const QString &msg)
{
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    const QString line = QString("[%1] [%2] %3").arg(ts, tag, msg);
    m_lines << line;
    // 每次只加一行，超出上限时删掉最早的一行即可（用 if 就够，不必 while）
    if (m_lines.size() > kMaxLines)
        m_lines.removeFirst();
    save();
}

// —— 各分类的便捷封装（都转调 append，只是分类标签不同）——
void Logger::info(const QString &msg)    { append("信息", msg); }   // 普通信息
void Logger::success(const QString &msg) { append("成功", msg); }   // 打开成功
void Logger::notepad(const QString &msg) { append("记事本", msg); } // 无默认程序，退回记事本打开

// 失败：可选附上「原因」后缀
void Logger::fail(const QString &msg, const QString &reason)
{
    QString text = msg;
    if (!reason.isEmpty())
        text += QString("  原因：%1").arg(reason);   // 有原因则附在末尾
    append("失败", text);
}

// 直接用一组现成记录覆盖内存中的记录（暂未使用，保留接口）
void Logger::setRaw(const QStringList &lines)
{
    m_lines = lines;
}

// 生成用于界面显示的文本（无记录时给一句占位提示）
QString Logger::toDisplayText() const
{
    if (m_lines.isEmpty())
        return QStringLiteral("（暂无记录）");
    return m_lines.join('\n');
}

// 清空全部记录并立即落盘
void Logger::clear()
{
    m_lines.clear();
    save();
}

// 把记录写入文件（UTF-8，覆盖式）
void Logger::save() const
{
    QFile f(kFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    for (const QString &line : m_lines)
        out << line << '\n';
    f.close();
}

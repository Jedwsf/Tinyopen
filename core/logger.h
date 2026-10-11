// =============================================================================
// core/logger.h — 后台记录器
// -----------------------------------------------------------------------------
// 把每次「打开文件/网址」的结果记录到本地 tinylog.txt，供「状况」按钮查看。
// 特点：只记录本次程序的运行情况（每次启动会清空上一次的记录）。
// 记录分类：信息 / 成功 / 失败 / 记事本（无默认程序时改用记事本打开）。
// =============================================================================

#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QStringList>

// 后台记录器
class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);   // 构造时清空旧记录

    void info(const QString &msg);                              // 普通信息
    void success(const QString &msg);                           // 打开成功
    void fail(const QString &msg, const QString &reason = QString());  // 打开失败（附原因）
    void notepad(const QString &msg);                           // 无默认程序，用记事本打开

    // 原始记录（含时间戳与分类），用于清空 / 持久化
    const QStringList &raw() const { return m_lines; }
    void setRaw(const QStringList &lines);   // 直接用一组记录覆盖内存中的记录

    // 供界面显示的可读文本（多行）
    QString toDisplayText() const;
    void clear();                      // 清空记录

    static const QString kFilePath();  // 记录文件路径（程序目录/tinylog.txt）

private:
    void append(const QString &tag, const QString &msg);   // 追加一行并保存
    void save() const;                                     // 写入文件

    QStringList m_lines;           // 全部记录行（内存副本，每行含时间戳+分类）
    const int kMaxLines = 3000;    // 记录上限，防止文件无限增长
};

#endif // LOGGER_H

// =============================================================================
// core/logger.h — 后台记录器
// =============================================================================
//
// 【在项目中的位置】
//   MainWindow 持有本对象，并把它传给 Launcher；Launcher 每打开一个文件/网址
//   就写一条记录。点工具栏「🕐 状况」时，MainWindow 用 toDisplayText() 取出文本，
//   交给 LogDialog 展示。
//
// 【职责】
//   把每次「打开文件 / 打开网址」的结果记录到本地 tinylog.txt。
//   特点：只记录「本次运行」——每次程序启动都会清空上一次的记录。
//
// 【记录格式】
//   每行： [yyyy-MM-dd HH:mm:ss] [分类] 正文
//   分类： 信息 / 成功 / 失败 / 记事本（无默认程序、退回记事本打开）
//
// 【类结构】
//   公有 ：info / success / fail / notepad   四个分类的记录入口
//          raw() / setRaw()                 原始记录存取
//          toDisplayText()                  供弹窗展示的多行文本
//          clear()                          清空全部记录
//          kFilePath()                      日志文件完整路径（静态）
//   私有 ：append(tag, msg) 追加一行并落盘；save() 写文件
//   成员 ：m_lines（内存中的全部行）、kMaxLines（上限 3000 行，防止无限增长）
//
// 【落盘策略】
//   每追加一行就整体重写一次 tinylog.txt —— 实现简单，且异常退出也不丢记录。
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

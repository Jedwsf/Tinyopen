// =============================================================================
// core/launcher.h — 启动器
// -----------------------------------------------------------------------------
// 负责真正「打开」一个条目：本地文件用系统默认程序打开，网页用默认浏览器打开。
// 底层统一调用 Windows 的 ShellExecuteW（Windows 会根据字符串形态自动分派）。
// 打开结果会写入日志（成功 / 失败 / 无默认程序退回记事本）。
// =============================================================================

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QObject>
#include <QString>
#include "groupdata.h"
#include "configstore.h"

class Logger;

// 启动器
class Launcher : public QObject
{
    Q_OBJECT
public:
    // logger 用于写打开结果；config 用于读取「启动间隔」
    Launcher(Logger *logger, ConfigStore *config, QObject *parent = nullptr);

    // 启动单个条目（文件或网址）；返回是否成功
    bool launch(const FileEntry &entry, const QString &groupName);

    // 启动整个分组（按序逐个启动组内全部文件）
    void launchGroup(const GroupEntry &group);

private:
    // 取条目的显示名（为空时回退为文件名）
    QString ensureDisplayName(const FileEntry &entry) const;

    Logger *m_logger;       // 写日志用
    ConfigStore *m_config;  // 读取「启动间隔」设置用
};

#endif // LAUNCHER_H

// =============================================================================
// core/launcher.h — 启动器（真正执行「打开」动作）
// =============================================================================
//
// 【在项目中的位置】
//   MainWindow 持有本对象。点分组的「▶」或某一行的「▶」时，最终都会走到这里。
//   它依赖 Logger（写结果）和 ConfigStore（读启动间隔）。
//
// 【职责】
//   把一个 FileEntry 真正打开：
//     - 本地文件 → 用系统默认程序打开
//     - 网址     → 用默认浏览器打开
//   并把成功 / 失败的结果写入日志。
//
// 【打开策略（launch）】
//   1) 路径为空                  → 记「失败：路径为空」
//   2) looksLikeUrl() 为真        → 直接交给默认浏览器（不检查存在性、不限长度）
//   3) 本地文件                   → 先按「启动间隔」等待
//                                  → 检查文件是否存在（不存在直接记失败）
//                                  → ShellExecuteW 用默认程序打开
//                                  → 若失败，退回用记事本打开
//                                  → 仍失败则记录可能的原因
//
// 【类结构】
//   公有 ：构造(logger, config) / launch(entry, groupName) / launchGroup(group)
//   私有 ：ensureDisplayName()  取日志里用的显示名
//          waitLaunchDelay()    按设置里的启动间隔等待
//   成员 ：m_logger、m_config
//
// 【关键判定】
//   ShellExecuteW 返回值 > 32 表示成功；<= 32 是错误码
//   （如 SE_ERR_ACCESSDENIED = 5、SE_ERR_NOASSOC = 31）。
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

    // 按设置里的「启动间隔」等待一段时间（0 表示不等待）
    void waitLaunchDelay() const;

    Logger *m_logger;       // 写日志用
    ConfigStore *m_config;  // 读取「启动间隔」设置用
};

#endif // LAUNCHER_H

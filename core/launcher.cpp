// =============================================================================
// core/launcher.cpp — 启动器实现
// =============================================================================
//
// 【打开流程（launch）】
//   1) 路径为空              → 记「失败：路径为空」
//   2) 网址                  → 等待启动间隔 → ShellExecuteW 交给默认浏览器
//                              （不检查文件存在性，也不受路径长度限制）
//   3) 本地文件              → 等待启动间隔 → 检查是否存在
//                              → ShellExecuteW 用默认程序打开
//                              → 失败则退回用记事本打开
//                              → 仍失败则按错误码记录原因
//
// 【核心 API】
//   ShellExecuteW(...) 返回值 > 32 表示成功；<= 32 是错误码
//   （SE_ERR_ACCESSDENIED = 5、SE_ERR_NOASSOC = 31 等）。
//   统一传 SW_SHOWNORMAL，保证目标窗口正常显示而不是最小化。
//
// 【函数一览】
//   Launcher()          保存 logger / config 指针
//   ensureDisplayName() 取日志用的显示名（displayName 为空则退回文件名）
//   waitLaunchDelay()   按设置里的启动间隔 msleep（0 表示不等待）
//   launch()            打开单个条目，返回是否成功
//   launchGroup()       按顺序打开一个分组里的全部条目
// =============================================================================

#include "launcher.h"
#include "logger.h"

#include <QDir>
#include <QFileInfo>
#include <QThread>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <shellapi.h>
#endif

// 构造：只保存日志器与配置的指针（启动时用于写日志、读启动间隔）
Launcher::Launcher(Logger *logger, ConfigStore *config, QObject *parent)
    : QObject(parent), m_logger(logger), m_config(config)
{
}

// 取显示名：优先用条目自带的 displayName，为空时退回文件名（仅用于日志显示）
QString Launcher::ensureDisplayName(const FileEntry &entry) const
{
    if (!entry.displayName.trimmed().isEmpty())
        return entry.displayName;
    return QFileInfo(entry.path).fileName();
}

// 按设置里的「启动间隔」等待一段时间（0 表示不等待）
void Launcher::waitLaunchDelay() const
{
    const int delay = m_config ? m_config->delayMs() : 0;
    if (delay > 0)
        QThread::msleep(static_cast<unsigned long>(delay));
}

// 启动单个条目（本地文件 / 网址），返回是否成功；过程与结果均写入日志
bool Launcher::launch(const FileEntry &entry, const QString &groupName)
{
    const QString name = ensureDisplayName(entry);
    const QString path = entry.path;

    // 1. 空路径
    if (path.trimmed().isEmpty()) {
        m_logger->fail(QString("分组[%1] 条目[%2] 启动失败").arg(groupName, name),
                       QStringLiteral("路径为空"));
        return false;
    }

    // 2. 网址 —— 用默认浏览器打开（不走文件存在性检查，也不受长度限制）
    if (looksLikeUrl(path)) {
        const QString url = normalizeUrl(path);

        waitLaunchDelay();   // 启动前的固定延迟（设置里的启动间隔）

#ifdef Q_OS_WIN
        const std::wstring wUrl = url.toStdWString();
        HINSTANCE r = ::ShellExecuteW(nullptr, L"open", wUrl.c_str(),
                                      nullptr, nullptr, SW_SHOWNORMAL);
        const INT_PTR code = reinterpret_cast<INT_PTR>(r);
        if (code > 32) {
            m_logger->success(QString("分组[%1] 打开 %2 （默认浏览器）网址：%3")
                                  .arg(groupName, name, url));
            return true;
        }
        m_logger->fail(QString("分组[%1] 打开 %2 失败 网址：%3").arg(groupName, name, url),
                       QStringLiteral("无法调用默认浏览器（可能未关联 http/https 协议）"));
        return false;
#else
        m_logger->fail(QString("分组[%1] 打开 %2 失败 网址：%3").arg(groupName, name, url),
                       QStringLiteral("当前平台暂不支持"));
        return false;
#endif
    }

    // 3. 启动前的固定延迟（设置里的启动间隔）
    waitLaunchDelay();

    const QFileInfo fi(path);
    const bool exists = fi.exists();

    // 4. 文件不存在 —— 直接失败，不必尝试打开
    if (!exists) {
        m_logger->fail(QString("分组[%1] 打开 %2 失败 路径：%3").arg(groupName, name, path),
                       QStringLiteral("文件不存在或路径无效"));
        return false;
    }

#ifdef Q_OS_WIN
    // 5. 用系统 shell 按默认程序打开（支持 exe/音乐/文档/文件夹等全部类型）
    const QString args = entry.arguments.trimmed();
    const std::wstring wPath = QDir::toNativeSeparators(path).toStdWString();
    const std::wstring wArgs = args.toStdWString();
    const std::wstring wDir  = QDir::toNativeSeparators(fi.absolutePath()).toStdWString();

    HINSTANCE r = ::ShellExecuteW(nullptr, L"open",
                                  wPath.c_str(),
                                  args.isEmpty() ? nullptr : wArgs.c_str(),
                                  wDir.empty() ? nullptr : wDir.c_str(),
                                  SW_SHOWNORMAL);

    // ShellExecuteW 返回值 > 32 表示成功
    const INT_PTR code = reinterpret_cast<INT_PTR>(r);
    if (code > 32) {
        m_logger->success(QString("分组[%1] 打开 %2 （默认程序）路径：%3")
                              .arg(groupName, name, path));
        return true;
    }

    // 6. 默认程序打开失败 —— 退回答记本
    HINSTANCE np = ::ShellExecuteW(nullptr, L"open", L"notepad.exe",
                                   wPath.c_str(), nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(np) > 32) {
        m_logger->notepad(QString("分组[%1] %2 无默认程序，已用记事本打开 路径：%3")
                              .arg(groupName, name, path));
        return true;
    }

    // 7. 记事本也失败 —— 记录可能原因
    QString reason;
    if (code == SE_ERR_ACCESSDENIED || reinterpret_cast<INT_PTR>(np) == SE_ERR_ACCESSDENIED)
        reason = QStringLiteral("权限不足，无法打开");
    else if (code == SE_ERR_NOASSOC)
        reason = QStringLiteral("无默认程序且记事本启动失败");
    else
        reason = QStringLiteral("无默认程序且记事本启动失败（可能权限不足或路径异常）");
    m_logger->fail(QString("分组[%1] 打开 %2 失败 路径：%3").arg(groupName, name, path), reason);
    return false;
#else
    m_logger->fail(QString("分组[%1] 打开 %2 失败 路径：%3").arg(groupName, name, path),
                   QStringLiteral("当前平台暂不支持"));
    return false;
#endif
}

// 逐个启动分组内的全部文件
void Launcher::launchGroup(const GroupEntry &group)
{
    for (const FileEntry &e : group.files)
        launch(e, group.name);
}

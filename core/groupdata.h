// =============================================================================
// core/groupdata.h — 核心数据结构
// -----------------------------------------------------------------------------
// 定义整个软件最基础的两个「数据模型」：
//   FileEntry  —— 一条文件/网址记录（显示名 + 路径 + 启动参数）
//   GroupEntry —— 一个分组（分组名 + 文件列表 + 是否开机自启）
// 另外提供两个网址识别的小工具函数，供「打开逻辑」和「图标绘制」共用。
// 本文件不依赖任何 Qt Widgets，纯数据 + 工具函数，可被任意模块包含。
// =============================================================================

#ifndef GROUPDATA_H
#define GROUPDATA_H

#include <QString>
#include <QList>

// 判断一个路径字符串是否应作为「网址」处理（用默认浏览器打开）。
// 识别：http:// https:// ftp:// ftps:// 以及以 www. 开头（自动补 https://）。
// 说明：与「本地文件路径」区分开，是决定用浏览器还是用系统默认程序的关键判断。
inline bool looksLikeUrl(const QString &raw)
{
    const QString s = raw.trimmed();
    if (s.isEmpty())
        return false;
    const QString lower = s.toLower();
    if (lower.startsWith(QLatin1String("http://"))
        || lower.startsWith(QLatin1String("https://"))
        || lower.startsWith(QLatin1String("ftp://"))
        || lower.startsWith(QLatin1String("ftps://"))
        || lower.startsWith(QLatin1String("www.")))
        return true;
    return false;
}

// 把用户输入的网址补全成可直接打开的地址（www. 开头补 https://）。
// 例如：www.baidu.com → https://www.baidu.com
inline QString normalizeUrl(const QString &raw)
{
    QString s = raw.trimmed();
    if (s.toLower().startsWith(QLatin1String("www.")))
        s.prepend(QLatin1String("https://"));
    return s;
}

// 单个文件条目：代表分组里的「一行」
struct FileEntry {
    QString displayName;   // 显示名称（留空时自动取文件名 / 网址）
    QString path;          // 本地文件完整路径，或网页地址（如 https://...）
    QString arguments;     // 启动参数（传给目标的命令行参数，一般留空）

    // 相等比较：用于判断条目是否被修改
    bool operator==(const FileEntry &o) const {
        return displayName == o.displayName && path == o.path && arguments == o.arguments;
    }
};

// 一个分组：包含若干文件条目
struct GroupEntry {
    QString name;              // 分组名（显示在分组标题栏）
    QList<FileEntry> files;    // 该分组下的全部文件条目
    bool autoLaunch = false;   // 软件启动时是否自动打开本分组
};

#endif // GROUPDATA_H

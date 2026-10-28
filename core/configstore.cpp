// =============================================================================
// core/configstore.cpp — 配置读写实现
// =============================================================================
//
// 【怎么存】
//   用 QJsonDocument 把内存里的 m_delayMs + m_groups 序列化成 JSON 文本，
//   写到程序目录的 config.json（缩进格式，方便人工查看/手改）。
//
// 【怎么读】
//   启动时读取并解析；文件不存在（首次运行）或解析失败时，保持默认值不报错。
//
// 【函数一览】
//   ConfigStore()  构造时立即 load() 一次
//   kFilePath()    返回「程序目录/config.json」
//   load()         读文件 → 解析 JSON → 填 m_delayMs 与 m_groups
//   save()         把 m_delayMs 与 m_groups 组装成 JSON → 覆盖写文件
// =============================================================================

#include "configstore.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

// 构造：立即读取一次本地配置（读不到就用默认值）
ConfigStore::ConfigStore(QObject *parent)
    : QObject(parent)
{
    load();   // 构造时先读取一次已有配置
}

// 配置文件完整路径 = 程序所在目录 + /config.json
const QString ConfigStore::kFilePath()
{
    // 配置放在程序所在文件夹（便携：整个文件夹拷走即可）
    return QCoreApplication::applicationDirPath() + "/config.json";
}

// 从 config.json 读取：启动间隔 + 全部分组（分组名 / 自启标记 / 文件条目）
void ConfigStore::load()
{
    QFile f(kFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        return;  // 首次运行，无配置文件，保持默认值
    }
    const QByteArray data = f.readAll();
    f.close();

    // 解析 JSON，出错则忽略（保留默认配置）
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    const QJsonObject root = doc.object();
    m_delayMs = root.value("delayMs").toInt(200);   // 缺省 200ms
    if (m_delayMs < 0)
        m_delayMs = 0;

    // 逐组、逐个文件还原
    m_groups.clear();
    const QJsonArray groups = root.value("groups").toArray();
    for (const QJsonValue &gv : groups) {
        const QJsonObject go = gv.toObject();
        GroupEntry g;
        g.name = go.value("name").toString();
        g.autoLaunch = go.value("autoLaunch").toBool(false);   // 缺省不自动打开
        const QJsonArray files = go.value("files").toArray();
        for (const QJsonValue &fv : files) {
            const QJsonObject fo = fv.toObject();
            FileEntry e;
            e.displayName = fo.value("displayName").toString();
            e.path        = fo.value("path").toString();
            e.arguments   = fo.value("arguments").toString();
            g.files.append(e);
        }
        m_groups.append(g);
    }
}

// 把当前配置（启动间隔 + 全部分组）写入 config.json（缩进格式，方便人工查看）
void ConfigStore::save() const
{
    QJsonObject root;
    root.insert("delayMs", m_delayMs);

    // 组装分组数组
    QJsonArray groups;
    for (const GroupEntry &g : m_groups) {
        QJsonObject go;
        go.insert("name", g.name);
        go.insert("autoLaunch", g.autoLaunch);

        QJsonArray files;
        for (const FileEntry &e : g.files) {
            QJsonObject fo;
            fo.insert("displayName", e.displayName);
            fo.insert("path", e.path);
            fo.insert("arguments", e.arguments);
            files.append(fo);
        }
        go.insert("files", files);
        groups.append(go);
    }
    root.insert("groups", groups);

    // 写入文件（缩进格式，方便人工查看）
    QFile f(kFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
}

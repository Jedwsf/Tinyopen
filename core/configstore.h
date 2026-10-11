// =============================================================================
// core/configstore.h — 配置读写
// -----------------------------------------------------------------------------
// 负责把「启动间隔」和「所有分组数据」持久化到本地 JSON 文件。
// 文件位置：程序所在文件夹下的 config.json（便携设计，整个文件夹拷走即可）。
// 注意：保存时机 = 内容变化时 / 设置修改时 / 主窗口关闭（析构）时。
// =============================================================================

#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QString>
#include <QList>
#include "groupdata.h"

// 应用配置管理器
class ConfigStore : public QObject
{
    Q_OBJECT
public:
    explicit ConfigStore(QObject *parent = nullptr);   // 构造时自动 load() 一次

    void load();                 // 从 config.json 读取配置
    void save() const;           // 把当前配置写入 config.json

    // —— 启动间隔（单个文件启动前的延迟，毫秒）——
    int delayMs() const { return m_delayMs; }
    void setDelayMs(int ms) { m_delayMs = ms; save(); }  // 改动后立即保存

    // —— 全部分组数据 ——
    QList<GroupEntry> groups() const { return m_groups; }
    void setGroups(const QList<GroupEntry> &groups) { m_groups = groups; }

    // 配置文件完整路径（= 程序目录 + /config.json）
    static const QString kFilePath();

signals:
    void delayChanged(int ms);   // 启动间隔变化时发出（预留：供界面同步用）

private:
    int m_delayMs = 200;             // 启动间隔默认值 200ms
    QList<GroupEntry> m_groups;      // 内存中的分组数据
};

#endif // CONFIGSTORE_H

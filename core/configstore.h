// =============================================================================
// core/configstore.h — 配置读写
// =============================================================================
//
// 【在项目中的位置】
//   MainWindow 持有本对象：启动时构造即自动 load()；之后每次界面变化，
//   由 MainWindow::onAnyChanged() 调 save() 落盘。
//
// 【职责】
//   把「启动间隔」和「所有分组数据」持久化到程序目录下的 config.json。
//   便携设计：配置跟着程序走，整个文件夹拷到别处照样能用。
//
// 【保存时机】
//   - 任何分组 / 文件改动（MainWindow::onAnyChanged）
//   - 修改启动间隔（setDelayMs 内部自动 save）
//   - 主窗口关闭（MainWindow 析构）
//   批量操作（启动还原、一次拖入多个文件）期间会跳过写盘，结束时统一保存一次。
//
// 【类结构】
//   公有 ：构造（自动 load）/ load() / save()
//          delayMs() / setDelayMs()       启动间隔
//          groups() / setGroups()         全部分组数据
//          kFilePath()                    配置文件完整路径（静态）
//   信号 ：delayChanged(int)              预留，目前未连接
//   成员 ：m_delayMs（默认 200 毫秒）、m_groups（内存中的分组数据）
//
// 【JSON 结构】
//   {
//     "delayMs": 200,
//     "groups": [
//       { "name": "…", "autoLaunch": false,
//         "files": [ { "displayName": "…", "path": "…", "arguments": "…" } ] }
//     ]
//   }
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
    // 返回 const 引用：避免每次读取都复制整个列表（调用方若要遍历时改动，请先自行取快照）
    const QList<GroupEntry> &groups() const { return m_groups; }
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

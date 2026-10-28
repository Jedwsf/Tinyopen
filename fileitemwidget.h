// =============================================================================
// fileitemwidget.h — 单个「文件行」
// =============================================================================
//
// 【界面结构】
//   ┌──────────────────────────────────────────────────────┐
//   │ [类型图标 20×20]  文件 / 网页名 ……   [▶ 启动] [🗑 删除] │
//   └──────────────────────────────────────────────────────┘
//   行高固定 38px；常态白底浅框，鼠标悬停变浅蓝底深框。
//   类型图标由 IconPainter::typeForPath() 按「网址 / 目录 / 扩展名」自动选择：
//     网页 → 地球、目录 → 文件夹、exe 等 → 程序、图片、音乐、文档、通用文件。
//
// 【职责】
//   只负责「显示一行」，并把用户动作变成信号，不决定动作本身怎么做：
//       ▶ 点击 → launchRequested(this)
//       🗑 点击 → deleteRequested(this)
//       双击行 → editRequested(this)
//   启动 / 删除 / 编辑的实际处理由 GroupWidget 接住，再转交 MainWindow。
//
// 【类结构】
//   公有    ：构造 / entry() / setEntry()
//   信号    ：launchRequested / deleteRequested / editRequested
//   受保护  ：mouseDoubleClickEvent（双击→编辑）、enterEvent / leaveEvent（悬停重绘）
//   私有    ：refresh() —— 按 m_entry 刷新图标、文字、按钮
//   成员    ：m_entry（本行数据）、m_iconLabel、m_nameLabel、m_launchBtn、m_deleteBtn
//
// 【注意】
//   本类用 QSS 画背景与边框，构造时必须 setAttribute(Qt::WA_StyledBackground, true)。
// =============================================================================

#ifndef FILEITEMWIDGET_H
#define FILEITEMWIDGET_H

#include <QWidget>
#include "core/groupdata.h"

class QLabel;
class QToolButton;

// 一行文件
class FileItemWidget : public QWidget
{
    Q_OBJECT
public:
    // entry 为本行要显示的文件/网址数据
    explicit FileItemWidget(const FileEntry &entry, QWidget *parent = nullptr);

    FileEntry entry() const { return m_entry; }
    void setEntry(const FileEntry &entry);   // 覆盖数据并刷新显示

signals:
    void launchRequested(FileItemWidget *self);   // 请求启动本行
    void deleteRequested(FileItemWidget *self);   // 请求删除本行
    void editRequested(FileItemWidget *self);     // 请求编辑本行（双击触发）

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;   // 双击 → 编辑
    void enterEvent(QEnterEvent *event) override;              // 悬停进入 → 重新抛光样式
    void leaveEvent(QEvent *event) override;                   // 悬停离开 → 重新抛光样式

private:
    void refresh();   // 根据 m_entry 刷新图标 / 文字 / 按钮

    FileEntry m_entry;   // 本行数据（显示名 / 路径或网址 / 启动参数）
    QLabel *m_iconLabel = nullptr;        // 左侧类型图标
    QLabel *m_nameLabel = nullptr;        // 文件 / 网页名
    QToolButton *m_launchBtn = nullptr;   // 启动按钮
    QToolButton *m_deleteBtn = nullptr;   // 删除按钮
};

#endif // FILEITEMWIDGET_H

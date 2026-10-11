// =============================================================================
// fileitemwidget.h — 单个文件行
// -----------------------------------------------------------------------------
// 一行布局：[类型图标] [文件/网页名 ......] [▶启动] [🗑删除]
// 交互：双击该行 → 请求编辑；鼠标悬停 → 高亮背景。
// 自身不决定「启动/删除」怎么做，只发出信号，交由上层 GroupWidget/MainWindow 处理。
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

// =============================================================================
// filedialog.h — 「文件设置 / 添加文件」弹窗
// -----------------------------------------------------------------------------
// 三个输入框：显示名称 / 路径或网址 / 启动参数。
//   isNew = true  → 标题「添加文件」（点分组加号时用）
//   isNew = false → 标题「文件设置」（双击文件行编辑时用）
// =============================================================================

#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <QDialog>
#include "core/groupdata.h"

class QLineEdit;
class QLabel;

// 文件设置弹窗
class FileDialog : public QDialog
{
    Q_OBJECT
public:
    // entry 为初始内容（新增时传空条目）；isNew=true 时标题为「添加文件」
    explicit FileDialog(const FileEntry &entry, QWidget *parent = nullptr, bool isNew = false);

    FileEntry result() const;   // 取用户填写/修改后的条目

private slots:
    void onAccept();   // 点「确定」：收集输入并关闭

private:
    QLineEdit *m_nameEdit = nullptr;   // 显示名称
    QLineEdit *m_pathEdit = nullptr;   // 路径或网址
    QLineEdit *m_argEdit  = nullptr;   // 启动参数

    FileEntry m_entry;   // 正在编辑的条目（确定时被更新）
};

#endif // FILEDIALOG_H

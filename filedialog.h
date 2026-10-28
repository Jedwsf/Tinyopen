// =============================================================================
// filedialog.h — 「添加文件 / 文件设置」弹窗
// =============================================================================
//
// 【界面结构】
//   ┌────────────────────────────────────────────┐
//   │ 显示名称   [______________________________] │
//   │ 路径/网址  [______________________________] │
//   │ 启动参数   [______________________________] │
//   │                            [确定]  [取消]   │
//   └────────────────────────────────────────────┘
//   同一个窗口服务两种用途，靠 isNew 决定标题：
//     isNew = true   → 「添加文件」：点分组右上角「＋」时用，初始内容为空
//     isNew = false  → 「文件设置」：双击已有文件行时用，带出原有内容
//
// 【职责】
//   只负责收集「显示名称 / 路径或网址 / 启动参数」三项输入，不做校验、不做打开。
//   路径框同时接受本地文件与网页地址，最终由 Launcher 按内容自动识别类型。
//
// 【类结构】
//   公有    ：构造(entry, parent, isNew) / result()
//   私有槽  ：onAccept()  点「确定」时把三个输入框的内容写回 m_entry
//   成员    ：m_nameEdit / m_pathEdit / m_argEdit / m_entry
//
// 【典型调用】
//     FileDialog dlg(g->fileAt(row), this);            // 编辑已有条目
//     if (dlg.exec() == QDialog::Accepted)
//         g->setFileAt(row, dlg.result());
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

// =============================================================================
// logdialog.h — 「后台记录」弹窗
// =============================================================================
//
// 【界面结构】
//   ┌──────────────────────────────────────────────┐
//   │ [2026-10-26 14:30:01] [成功] 分组[x] 打开 …   │
//   │ [2026-10-26 14:30:02] [失败] 分组[x] 打开 …   │
//   │ …（只读，可滚动）                              │
//   ├──────────────────────────────────────────────┤
//   │ 记录保存于本地 tinylog.txt          [关闭]     │
//   └──────────────────────────────────────────────┘
//
// 【职责】
//   纯展示窗口：把 Logger::toDisplayText() 生成的文本放进只读文本框。
//   不读文件、不写文件，日志内容的产生与落盘都由 core/logger 负责。
//
// 【类结构】
//   公有 ：构造(text, parent)
//   成员 ：m_view（QPlainTextEdit，只读，等宽字体便于看时间戳）
//
// 【典型调用】
//     LogDialog dlg(m_logger->toDisplayText(), this);
//     dlg.exec();
// =============================================================================

#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include <QDialog>

class QPlainTextEdit;

// 后台记录窗口
class LogDialog : public QDialog
{
    Q_OBJECT
public:
    // text 为要展示的后台记录文本（由 Logger::toDisplayText() 生成）
    explicit LogDialog(const QString &text, QWidget *parent = nullptr);

private:
    QPlainTextEdit *m_view = nullptr;   // 只读日志展示区
};

#endif // LOGDIALOG_H

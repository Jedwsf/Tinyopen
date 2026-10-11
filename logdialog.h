// =============================================================================
// logdialog.h — 后台记录弹窗
// -----------------------------------------------------------------------------
// 点击工具栏「状况」按钮后打开，只读展示本次运行的全部打开记录。
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

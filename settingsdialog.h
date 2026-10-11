// =============================================================================
// settingsdialog.h — 设置弹窗
// -----------------------------------------------------------------------------
// 目前包含：启动间隔（单个文件启动前的延迟，毫秒）+ 版本号。
// =============================================================================

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSpinBox;

// 设置弹窗
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    // delayMs 为当前的启动间隔初始值
    explicit SettingsDialog(int delayMs, QWidget *parent = nullptr);

    int delayMs() const;   // 取用户设置的启动间隔

private:
    QSpinBox *m_delaySpin = nullptr;   // 启动间隔（毫秒）
};

#endif // SETTINGSDIALOG_H

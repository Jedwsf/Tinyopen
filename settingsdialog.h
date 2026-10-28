// =============================================================================
// settingsdialog.h — 「设置」弹窗
// =============================================================================
//
// 【界面结构】
//   ┌────────────────────────────────────┐
//   │ 启动间隔  [  200 毫秒 ⌃⌄ ]          │
//   │ 每次启动某个文件前，固定等待这段时间。│
//   │ ────────────────────────────────   │
//   │ 更多新功能正在开发中                │
//   │ 版本 1.2.1                         │
//   │                     [确定]  [取消]  │
//   └────────────────────────────────────┘
//
// 【职责】
//   只负责编辑「启动间隔」这一个设置项，并展示当前版本号。
//   设置项本身由 ConfigStore 持有和保存，本类不碰文件。
//
// 【类结构】
//   公有    ：构造(delayMs, parent) / delayMs()
//   成员    ：m_delaySpin（QSpinBox，0～60000 毫秒，步进 50）
//
// 【典型调用】
//     SettingsDialog dlg(m_config->delayMs(), this);
//     if (dlg.exec() == QDialog::Accepted)
//         m_config->setDelayMs(dlg.delayMs());   // setDelayMs 内部会 save()
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

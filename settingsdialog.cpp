// =============================================================================
// settingsdialog.cpp — 设置弹窗实现
// -----------------------------------------------------------------------------
// 布局：启动间隔输入框 + 说明 → 分隔线 → 「开发中」提示 + 版本号 → 确定/取消。
// =============================================================================

#define VERSION "版本 1.2.1"

#include "settingsdialog.h"

#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>

// 构造：搭建「启动间隔」输入框 + 说明 + 版本号 + 确定/取消
SettingsDialog::SettingsDialog(int delayMs, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("设置"));
    setModal(true);
    setMinimumWidth(340);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 18, 20, 16);
    mainLayout->setSpacing(14);

    // ---------- 启动间隔 ----------
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setSpacing(10);

    m_delaySpin = new QSpinBox(this);
    m_delaySpin->setRange(0, 60000);                 // 0 ~ 60 秒
    m_delaySpin->setSingleStep(50);                  // 步进 50ms
    m_delaySpin->setSuffix(QStringLiteral(" 毫秒"));
    m_delaySpin->setValue(delayMs);
    m_delaySpin->setFixedWidth(140);

    form->addRow(QStringLiteral("启动间隔"), m_delaySpin);
    mainLayout->addLayout(form);

    // 说明文字
    auto *hint = new QLabel(QStringLiteral("每次启动某个文件前，固定等待这段时间再执行。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#8a8a8a;font-size:12px;");
    mainLayout->addWidget(hint);

    // 分隔线
    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color:#e2e2e2;");
    mainLayout->addWidget(line);

    // 新功能提示
    auto *devLabel = new QLabel(QStringLiteral("更多新功能正在开发中"), this);
    devLabel->setStyleSheet("color:#9a9a9a;font-size:12px;");
    mainLayout->addWidget(devLabel);

    // 版本号
    auto *verLabel = new QLabel(QStringLiteral(VERSION), this);
    verLabel->setStyleSheet("color:#9a9a9a;font-size:12px;");
    mainLayout->addWidget(verLabel);

    mainLayout->addStretch();

    // ---------- 按钮 ----------
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto *okBtn = new QPushButton(QStringLiteral("确定"), this);
    auto *cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    okBtn->setDefault(true);
    okBtn->setFixedWidth(76);
    cancelBtn->setFixedWidth(76);
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    mainLayout->addLayout(btnRow);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

// 取用户设置的启动间隔（毫秒）
int SettingsDialog::delayMs() const
{
    return m_delaySpin->value();
}

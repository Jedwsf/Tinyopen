// =============================================================================
// filedialog.cpp — 「文件设置 / 添加文件」弹窗实现
// -----------------------------------------------------------------------------
// 路径框同时接受「本地文件路径」和「网页地址」，由 Launcher 按内容自动识别。
// =============================================================================

#include "filedialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

// 构造：搭建「显示名称 / 路径-网址 / 启动参数」三个输入行 + 确定/取消
// 参数 isNew=true 表示「添加文件」（空白条目），false 表示编辑已有条目
FileDialog::FileDialog(const FileEntry &entry, QWidget *parent, bool isNew)
    : QDialog(parent), m_entry(entry)
{
    // 根据用途切换标题：新增 / 编辑
    setWindowTitle(isNew ? QStringLiteral("添加文件") : QStringLiteral("文件设置"));
    setModal(true);
    setMinimumWidth(460);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 18, 20, 16);
    mainLayout->setSpacing(12);

    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 显示名称（可留空，留空时自动取文件名/网址）
    m_nameEdit = new QLineEdit(entry.displayName, this);
    m_nameEdit->setPlaceholderText(QStringLiteral("不填则显示文件名"));

    // 路径 / 网址
    m_pathEdit = new QLineEdit(entry.path, this);
    m_pathEdit->setPlaceholderText(QStringLiteral("文件完整路径，或网页地址（如 https://www.bilibili.com）"));

    // 启动参数
    m_argEdit = new QLineEdit(entry.arguments, this);
    m_argEdit->setPlaceholderText(QStringLiteral("可留空，例如：--fullscreen \"C:\\a b.txt\""));

    form->addRow(QStringLiteral("显示名称"), m_nameEdit);
    form->addRow(QStringLiteral("路径/网址"), m_pathEdit);
    form->addRow(QStringLiteral("启动参数"), m_argEdit);
    mainLayout->addLayout(form);

    mainLayout->addStretch();

    // 底部按钮：确定 / 取消
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

    connect(okBtn, &QPushButton::clicked, this, &FileDialog::onAccept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

// 收集输入：显示名去首尾空格，路径原样保留
void FileDialog::onAccept()
{
    m_entry.displayName = m_nameEdit->text().trimmed();
    m_entry.path        = m_pathEdit->text();
    m_entry.arguments   = m_argEdit->text();
    accept();
}

// 取用户填写/修改后的条目（点「确定」后由调用方读取）
FileEntry FileDialog::result() const
{
    return m_entry;
}

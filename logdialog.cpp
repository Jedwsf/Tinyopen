// =============================================================================
// logdialog.cpp — 后台记录弹窗实现
// -----------------------------------------------------------------------------
// 只读文本框展示日志 + 底部「记录保存于本地 tinylog.txt」说明 + 关闭按钮。
// =============================================================================

#include "logdialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

// 构造：只读文本框展示日志 + 底部「记录保存于本地 tinylog.txt」+ 关闭按钮
LogDialog::LogDialog(const QString &text, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("后台记录"));
    setModal(true);
    resize(620, 440);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 14);
    mainLayout->setSpacing(10);

    // 只读日志文本框（等宽字体，方便阅读时间戳）
    m_view = new QPlainTextEdit(this);
    m_view->setReadOnly(true);
    m_view->setPlainText(text);
    m_view->setStyleSheet(
        "QPlainTextEdit{background:#fbfbfb;border:1px solid #e2e2e2;border-radius:6px;"
        "padding:8px;font-family:Consolas,'Microsoft YaHei',monospace;font-size:12px;}");
    mainLayout->addWidget(m_view);

    // 底部：说明 + 关闭按钮
    auto *btnRow = new QHBoxLayout;
    auto *hint = new QLabel(QStringLiteral("记录保存于本地 tinylog.txt"), this);
    hint->setStyleSheet("color:#9a9a9a;font-size:12px;");
    btnRow->addWidget(hint);
    btnRow->addStretch();

    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    closeBtn->setFixedWidth(76);
    btnRow->addWidget(closeBtn);
    mainLayout->addLayout(btnRow);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

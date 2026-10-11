// =============================================================================
// fileitemwidget.cpp — 单个文件行实现
// -----------------------------------------------------------------------------
// 样式：白底小卡片 + 浅边框，鼠标悬停变浅蓝底深边框（需要开启 WA_StyledBackground）。
// =============================================================================

#include "fileitemwidget.h"
#include "core/iconpainter.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>

// 图标颜色
static const QColor kIconColor(0x55, 0x5f, 0x6b);     // 类型图标
static const QColor kAccentColor(0x2f, 0x7d, 0xd1);   // 启动按钮
static const QColor kDangerColor(0xc0, 0x4a, 0x4a);   // 删除按钮

// 构造：搭建一行 [类型图标 | 名称 | 启动 | 删除]，并接好两个按钮的信号
FileItemWidget::FileItemWidget(const FileEntry &entry, QWidget *parent)
    : QWidget(parent), m_entry(entry)
{
    setObjectName("fileRow");
    setAttribute(Qt::WA_StyledBackground, true);   // 关键：让 QSS 背景/边框生效
    setFixedHeight(38);                            // 行高固定
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 8, 0);
    layout->setSpacing(8);

    // 左侧类型图标
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(20, 20);

    // 中间名称
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-size:13px;color:#2b2b2b;");

    // 右侧启动按钮
    m_launchBtn = new QToolButton(this);
    m_launchBtn->setToolTip(QStringLiteral("启动"));
    m_launchBtn->setAutoRaise(true);
    m_launchBtn->setFixedSize(26, 26);

    // 右侧删除按钮
    m_deleteBtn = new QToolButton(this);
    m_deleteBtn->setToolTip(QStringLiteral("删除"));
    m_deleteBtn->setAutoRaise(true);
    m_deleteBtn->setFixedSize(26, 26);

    layout->addWidget(m_iconLabel);
    layout->addWidget(m_nameLabel, 1);   // 名称占据剩余宽度
    layout->addWidget(m_launchBtn);
    layout->addWidget(m_deleteBtn);

    // 行样式：常态白底浅框；悬停浅蓝底深框
    setStyleSheet(
        "QWidget#fileRow{background:#ffffff;border:1px solid #dfe5ee;border-radius:6px;}"
        "QWidget#fileRow:hover{background:#f2f6fb;border:1px solid #b9cdea;}");

    // 按钮点击 → 仅发信号，具体处理交给上层
    connect(m_launchBtn, &QToolButton::clicked, this, [this] { emit launchRequested(this); });
    connect(m_deleteBtn, &QToolButton::clicked, this, [this] { emit deleteRequested(this); });

    refresh();
}

// 根据 m_entry 刷新本行显示
void FileItemWidget::refresh()
{
    const QString path = m_entry.path;

    // 1) 类型图标（网页 → 地球；按扩展名区分图片/音乐/程序/文档）
    const IconPainter::IconType t = IconPainter::typeForPath(path);
    m_iconLabel->setPixmap(IconPainter::make(t, kIconColor, 20).pixmap(20, 20));

    // 2) 显示名：优先用自带的 displayName，否则网址用地址、文件用文件名
    QString name = m_entry.displayName.trimmed();
    if (name.isEmpty()) {
        if (looksLikeUrl(path))
            name = normalizeUrl(path);          // 网址直接用地址当名字
        else
            name = QFileInfo(path).fileName();
    }
    if (name.isEmpty())
        name = QStringLiteral("(未命名)");
    m_nameLabel->setText(name);
    m_nameLabel->setToolTip(looksLikeUrl(path) ? normalizeUrl(path) : path);

    // 3) 按钮图标
    m_launchBtn->setIcon(IconPainter::make(IconPainter::Play, kAccentColor, 16));
    m_launchBtn->setIconSize(QSize(16, 16));

    m_deleteBtn->setIcon(IconPainter::make(IconPainter::Trash, kDangerColor, 16));
    m_deleteBtn->setIconSize(QSize(16, 16));
}

// 覆盖本行数据并立即刷新显示（编辑弹窗确认后调用）
void FileItemWidget::setEntry(const FileEntry &entry)
{
    m_entry = entry;
    refresh();
}

// 双击 → 请求编辑
void FileItemWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit editRequested(this);
}

// 鼠标移入：重新抛光样式，让 :hover 立即生效
void FileItemWidget::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    style()->unpolish(this);
    style()->polish(this);
}

// 鼠标移出：同上
void FileItemWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    style()->unpolish(this);
    style()->polish(this);
}

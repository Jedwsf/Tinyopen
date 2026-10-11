// =============================================================================
// groupwidget.cpp — 分组面板实现
// -----------------------------------------------------------------------------
// 布局：根布局 root [ 标题栏 headerBar(名称+4个按钮) | 文件区 rowHost(文件行) ]
// 样式用 QSS 描述（白底圆角 + 标题栏浅蓝底），因此自定义 QWidget 必须开启
// WA_StyledBackground 才能绘制背景/边框。
// =============================================================================

#include "groupwidget.h"
#include "fileitemwidget.h"
#include "core/iconpainter.h"

#include <QApplication>
#include <QDir>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QToolButton>
#include <QVBoxLayout>

// 各类按钮的图标颜色
static const QColor kIconColor(0x55, 0x5f, 0x6b);     // 常规灰蓝
static const QColor kAccentColor(0x2f, 0x7d, 0xd1);   // 强调蓝（启动）
static const QColor kDangerColor(0xc0, 0x4a, 0x4a);   // 危险红（删除）
static const QColor kAutoOnColor(0x2e, 0x9e, 0x5b);   // 开机自启开启时：绿色

GroupWidget::GroupWidget(const GroupEntry &entry, QWidget *parent)
    : QWidget(parent), m_entry(entry)
{
    setObjectName("groupPanel");
    setAcceptDrops(true);                                 // 允许拖文件进本分组
    setAttribute(Qt::WA_StyledBackground, true);          // 关键：让 QSS 背景/边框生效
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);  // 高度贴合内容
    setMinimumHeight(120);                                // 分组最小高度

    // 分组边框 + 标题栏底色（方案 A 样式）
    setStyleSheet(
        "QWidget#groupPanel{background:#ffffff;border:1.5px solid #aab4c2;border-radius:10px;}"
        "QWidget#groupHeader{background:#eef3fa;border-top-left-radius:10px;border-top-right-radius:10px;"
        "border-bottom:1px solid #d8e2ee;}");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---------- 顶部：左名字 / 右按钮（带浅蓝底色的标题栏） ----------
    auto *headerBar = new QWidget(this);
    headerBar->setObjectName("groupHeader");
    headerBar->setAttribute(Qt::WA_StyledBackground, true);
    m_headerBar = headerBar;                              // 记住标题栏，用于拖动整组
    headerBar->setCursor(Qt::OpenHandCursor);             // 提示：这里可以按住拖动
    auto *header = new QHBoxLayout(headerBar);
    header->setContentsMargins(14, 10, 14, 10);
    header->setSpacing(6);

    m_nameLabel = new QLabel(m_entry.name, this);
    m_nameLabel->setStyleSheet("font-size:14px;font-weight:600;color:#22303f;");
    m_nameLabel->setCursor(Qt::PointingHandCursor);
    m_nameLabel->setToolTip(QStringLiteral("点击修改分组名"));
    m_nameLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    m_nameEdit = new QLineEdit(m_entry.name, this);
    m_nameEdit->setStyleSheet(
        "QLineEdit{font-size:14px;font-weight:600;color:#22303f;"
        "border:1px solid #2f7dd1;border-radius:5px;padding:1px 4px;background:#fff;}");
    m_nameEdit->setFixedWidth(160);
    m_nameEdit->hide();

    m_addBtn    = new QToolButton(this);
    m_launchBtn = new QToolButton(this);
    m_deleteBtn = new QToolButton(this);
    m_autoBtn   = new QToolButton(this);

    struct BtnDef { QToolButton *btn; IconPainter::IconType type; const char *tip; QColor color; };
    const BtnDef defs[] = {
        { m_addBtn,    IconPainter::Plus,  "添加文件", kIconColor   },
        { m_launchBtn, IconPainter::Play,  "启动分组", kAccentColor },
        { m_deleteBtn, IconPainter::Trash, "删除分组", kDangerColor },
    };
    for (const BtnDef &d : defs) {
        d.btn->setToolTip(QString::fromUtf8(d.tip));
        d.btn->setAutoRaise(true);
        d.btn->setFixedSize(28, 28);
        d.btn->setIcon(IconPainter::make(d.type, d.color, 18));
        d.btn->setIconSize(QSize(18, 18));
    }

    // 开机自启开关：勾选后软件启动时自动打开本分组
    m_autoBtn->setCheckable(true);
    m_autoBtn->setAutoRaise(true);
    m_autoBtn->setFixedSize(28, 28);
    m_autoBtn->setIconSize(QSize(18, 18));
    m_autoBtn->setCursor(Qt::PointingHandCursor);
    m_autoBtn->setStyleSheet(
        "QToolButton{border-radius:6px;}"
        "QToolButton:hover{background:#e6eefa;}");

    header->addWidget(m_nameLabel);
    header->addWidget(m_nameEdit);
    header->addStretch();
    header->addWidget(m_autoBtn);
    header->addWidget(m_addBtn);
    header->addWidget(m_launchBtn);
    header->addWidget(m_deleteBtn);
    root->addWidget(headerBar);

    // ---------- 文件名列表 ----------
    auto *rowHost = new QWidget(this);
    auto *rowHostLayout = new QVBoxLayout(rowHost);
    rowHostLayout->setContentsMargins(10, 8, 10, 10);
    rowHostLayout->setSpacing(0);
    root->addWidget(rowHost);

    m_rowLayout = new QVBoxLayout;
    m_rowLayout->setContentsMargins(0, 0, 0, 0);
    m_rowLayout->setSpacing(4);           // 文件行之间的间距
    rowHostLayout->addLayout(m_rowLayout);

    // 空提示（分组内没有任何文件时显示）
    m_emptyHint = new QLabel(QStringLiteral("把文件拖到这里，或点击右上角加号添加"), this);
    m_emptyHint->setStyleSheet("color:#a7b0ba;font-size:12px;padding:10px 4px;");
    m_rowLayout->addWidget(m_emptyHint);

    // 让文件行始终贴顶，避免分组被撑高时出现大片空白
    m_rowLayout->addStretch();

    rebuildRows();

    // 点击分组名可改名；按住标题栏（或分组名）拖动可给整个分组换位置
    m_nameLabel->installEventFilter(this);
    m_headerBar->installEventFilter(this);
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &GroupWidget::onNameEdited);
    connect(m_addBtn,    &QToolButton::clicked, this, [this] { emit requestAddFiles(this); });
    connect(m_launchBtn, &QToolButton::clicked, this, [this] { emit requestLaunch(this); });
    connect(m_deleteBtn, &QToolButton::clicked, this, [this] { emit requestDelete(this); });
    // 开机自启开关被切换：写回数据 + 刷新外观 + 触发保存
    connect(m_autoBtn,   &QToolButton::toggled, this, [this](bool on) {
        m_entry.autoLaunch = on;
        updateAutoButton();
        emit changed();
    });

    updateAutoButton();
    updateEmptyHint();
}

// 根据当前状态刷新「开机自启」按钮的图标颜色与提示
void GroupWidget::updateAutoButton()
{
    const bool on = m_entry.autoLaunch;
    m_autoBtn->setChecked(on);
    m_autoBtn->setIcon(IconPainter::make(IconPainter::Power,
                                         on ? kAutoOnColor : kIconColor, 18));
    m_autoBtn->setToolTip(on
        ? QStringLiteral("本分组会在软件启动时自动打开（点击取消）")
        : QStringLiteral("开机自启：勾选后软件启动时自动打开本分组"));
}

// 设置开机自启（供外部调用；无变化则不动）
void GroupWidget::setAutoLaunch(bool on)
{
    if (m_entry.autoLaunch == on)
        return;
    m_entry.autoLaunch = on;
    updateAutoButton();
    emit changed();
}

// 清空并重建全部文件行（从 m_entry.files 生成）
void GroupWidget::rebuildRows()
{
    for (FileItemWidget *w : m_rows) {
        m_rowLayout->removeWidget(w);
        w->deleteLater();
    }
    m_rows.clear();

    for (const FileEntry &e : m_entry.files)
        appendRow(e);
    updateEmptyHint();
}

// 追加一行文件行，并把它的事件连接到本类的处理逻辑
void GroupWidget::appendRow(const FileEntry &entry)
{
    auto *row = new FileItemWidget(entry, this);
    m_rows.append(row);

    // 插到空提示之前
    const int idx = m_rowLayout->indexOf(m_emptyHint);
    m_rowLayout->insertWidget(idx >= 0 ? idx : m_rowLayout->count(), row);

    // 点该行「启动」→ 把最新数据写回后，交由主窗口启动（主窗口持有 Logger）
    connect(row, &FileItemWidget::launchRequested, this, [this](FileItemWidget *r) {
        const int i = rowIndex(r);
        if (i >= 0)
            m_entry.files[i] = r->entry();
        emit requestLaunchFile(this, i);
    });
    // 点该行「删除」→ 从数据与界面同时移除
    connect(row, &FileItemWidget::deleteRequested, this, [this](FileItemWidget *r) {
        const int i = rowIndex(r);
        if (i < 0)
            return;
        m_entry.files.removeAt(i);
        m_rows.removeAt(i);
        m_rowLayout->removeWidget(r);
        r->deleteLater();
        updateEmptyHint();
        emit changed();
    });
    // 双击该行 → 请求主窗口弹出编辑窗
    connect(row, &FileItemWidget::editRequested, this, [this](FileItemWidget *r) {
        const int i = rowIndex(r);
        if (i >= 0)
            emit requestEditFile(this, i);  // 由主窗口弹出编辑弹窗
    });
}

// 由控件指针反查其在文件列表中的下标
int GroupWidget::rowIndex(const FileItemWidget *row) const
{
    return m_rows.indexOf(const_cast<FileItemWidget *>(row));
}

// 读第 index 行数据（越界返回空条目）
FileEntry GroupWidget::fileAt(int index) const
{
    if (index < 0 || index >= m_entry.files.size())
        return FileEntry();
    return m_entry.files.at(index);
}

// 覆盖第 index 行数据，并同步刷新该行界面
void GroupWidget::setFileAt(int index, const FileEntry &entry)
{
    if (index < 0 || index >= m_entry.files.size())
        return;
    m_entry.files[index] = entry;
    if (index < m_rows.size())
        m_rows.at(index)->setEntry(entry);
    emit changed();
}

// 拖放路径批量添加（路径来自资源管理器，必然是本地文件）
void GroupWidget::addFiles(const QStringList &paths)
{
    const bool wasEmpty = m_entry.files.isEmpty();
    for (const QString &path : paths) {
        FileEntry e;
        e.path = QDir::toNativeSeparators(path);
        e.displayName = QFileInfo(path).fileName();
        m_entry.files.append(e);
        appendRow(e);
    }
    updateEmptyHint();
    if (wasEmpty && !m_entry.files.isEmpty())
        maybeAutoRename(m_entry.files.first());   // 加入首个文件 → 自动改名
    emit changed();
}

// 手动新增一条（来自「添加文件」弹窗；可能是文件也可能是网址）
void GroupWidget::addFile(const FileEntry &entry)
{
    const bool wasEmpty = m_entry.files.isEmpty();
    FileEntry e = entry;
    if (looksLikeUrl(e.path)) {
        // 网址：原样保留（不做 toNativeSeparators，否则会把 / 变成 \）
        e.path = normalizeUrl(e.path);
        if (e.displayName.trimmed().isEmpty())
            e.displayName = e.path;
    } else {
        e.path = QDir::toNativeSeparators(e.path);
        if (e.displayName.trimmed().isEmpty())
            e.displayName = QFileInfo(e.path).fileName();
    }
    m_entry.files.append(e);
    appendRow(e);
    updateEmptyHint();
    if (wasEmpty)
        maybeAutoRename(e);   // 加入首个文件 → 自动改名
    emit changed();
}

// 若分组名仍是默认「新分组」，用首个文件（或网址域名）作为新分组名
void GroupWidget::maybeAutoRename(const FileEntry &firstAdded)
{
    // 仅在分组名仍为默认「新分组」时自动改名，避免覆盖用户自定义的名称
    if (m_entry.name != defaultGroupName())
        return;

    // 网址：直接用地址作为分组名（不做去扩展名处理）
    if (looksLikeUrl(firstAdded.path)) {
        QString host = normalizeUrl(firstAdded.path);
        host.remove(QRegularExpression(QStringLiteral("^[a-zA-Z]+://")));
        host = host.section(QLatin1Char('/'), 0, 0);   // 取域名部分
        if (!host.isEmpty()) {
            m_entry.name = host;
            m_nameLabel->setText(host);
            m_nameEdit->setText(host);
        }
        return;
    }

    QString base = firstAdded.displayName.trimmed();
    if (base.isEmpty())
        base = QFileInfo(firstAdded.path).fileName();
    if (base.isEmpty())
        return;

    // 去掉扩展名，作为更清爽的分组名（如 LingChat.lnk → LingChat）
    const QString suffix = QFileInfo(base).suffix();
    if (!suffix.isEmpty() && base.endsWith(QLatin1Char('.') + suffix))
        base.chop(suffix.length() + 1);
    if (base.isEmpty())
        base = firstAdded.displayName.trimmed();

    // 名字最长 24 字符，超出则截断
    if (base.length() > 24)
        base = base.left(24);

    m_entry.name = base;
    m_nameLabel->setText(base);
    m_nameEdit->setText(base);
}

// 从配置还原整个分组（分组名 + 自启标记 + 全部文件行）
void GroupWidget::loadGroup(const GroupEntry &entry)
{
    m_entry = entry;
    m_nameLabel->setText(m_entry.name);
    m_nameEdit->setText(m_entry.name);
    updateAutoButton();
    rebuildRows();
}

// 取出本分组的完整数据（主窗口保存配置 / 启动分组时调用）
GroupEntry GroupWidget::entry() const
{
    return m_entry;
}

// 空提示只在没有任何文件时显示
void GroupWidget::updateEmptyHint()
{
    m_emptyHint->setVisible(m_entry.files.isEmpty());
}

// 点击分组名：隐藏展示标签，显示行内编辑框并全选，等待用户输入
void GroupWidget::onRenameClicked()
{
    m_nameLabel->hide();
    m_nameEdit->setText(m_entry.name);
    m_nameEdit->show();
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

// 改名结束（回车/失焦）：写入数据、切回展示态、触发保存
void GroupWidget::onNameEdited()
{
    if (!m_nameEdit->isVisible())
        return;
    const QString text = m_nameEdit->text().trimmed();
    if (!text.isEmpty()) {
        m_entry.name = text;
        m_nameLabel->setText(text);
    }
    m_nameEdit->hide();
    m_nameLabel->show();
    emit changed();
}

// 事件过滤器：点击分组名进入改名；按住标题栏 / 分组名拖动整个分组
bool GroupWidget::eventFilter(QObject *obj, QEvent *event)
{
    // —— 拖动整组：在标题栏（或分组名）上按住鼠标并移动超过阈值 ——
    if (obj == m_headerBar || obj == m_nameLabel) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton)
                m_headerPressPos = me->pos();          // 记下按下起点
        } else if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(event);
            // 左键按住且移动距离超过系统拖动阈值 → 发起拖动
            if ((me->buttons() & Qt::LeftButton)
                && (me->pos() - m_headerPressPos).manhattanLength()
                       >= QApplication::startDragDistance()) {
                startGroupDrag();
                return true;
            }
        }
    }

    // —— 点击分组名 → 进入改名（只有「没被当成拖动」的普通点击才会走到这） ——
    if (obj == m_nameLabel && event->type() == QEvent::MouseButtonRelease) {
        onRenameClicked();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

// 用标题栏拖动整个分组：发起一个带自定义 MIME 的 QDrag，
// 主窗口收到后据此把本分组在列表里挪位置（而不是当成「拖入文件」）。
void GroupWidget::startGroupDrag()
{
    auto *drag = new QDrag(this);
    auto *mime = new QMimeData;
    mime->setData(QString::fromLatin1(TINYOPEN_GROUP_MIME), QByteArrayLiteral("1"));
    drag->setMimeData(mime);

    // 刻意不设置 drag->setPixmap(...)：拖动时不显示跟随鼠标的分组快照，
    // 只靠主窗口显示的「蓝色插入指示线」来表示落点。

    emit dragStarted(this);        // 通知主窗口：正在拖动本分组
    drag->exec(Qt::MoveAction);    // 阻塞，直到松手
    emit dragFinished();           // 通知主窗口：拖动结束，隐藏指示线
}

// 拖拽进入：有 URL（拖入文件）才接受；分组换位置的拖拽交给主窗口处理
void GroupWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

// 拖拽放下：把本地文件加入本分组
void GroupWidget::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return;

    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile())
            paths << url.toLocalFile();
    }
    if (paths.isEmpty())
        return;

    // 拖到分组里就直接新增文件行（不弹窗）
    addFiles(paths);
    event->acceptProposedAction();
}

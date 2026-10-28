// =============================================================================
// mainwindow.cpp — 主窗口实现（整个程序的调度中枢）
// =============================================================================
//
// 【在项目中的位置】
//   main.cpp 创建本窗口。它向下持有 GroupWidget（分组面板）与三个弹窗，
//   向内持有 ConfigStore / Logger / Launcher 三个 core 模块。
//   几乎所有跨模块的动作都经过这里：
//       面板发出请求信号 → 本类处理 → 调 core 模块或弹对话框 → 变化后统一保存。
//
// 【界面结构】
//   QMainWindow
//   └─ central (QWidget，浅灰底 #f7f8fa)
//      └─ QVBoxLayout root
//         ├─ toolbar（48px 高）
//         │     [Tinyopen] ................ [+ 新建分组] [⚙ 设置] [🕐 状况]
//         └─ QScrollArea m_scroll（可滚动）
//            └─ m_groupHost (QWidget)          ← 接收拖拽的容器
//               └─ QVBoxLayout m_groupLayout（边距 16/14/16/16，间距 12）
//                  ├─ GroupWidget …            ← 每个分组一块面板
//                  ├─ m_dropIndicator (QFrame) ← 拖动时的蓝色插入线（平时不在布局里）
//                  └─ addStretch()             ← 末尾拉伸，让分组从顶部开始排
//
// 【成员与职责】
//   m_config          ConfigStore  读取启动间隔、读写全部分组数据
//   m_logger          Logger       记录每次打开的结果
//   m_launcher        Launcher     执行真正的打开动作
//   m_dragGroup       正在被拖动的分组（仅拖动期间非空）
//   m_batchDepth      >0 表示批量操作中：只同步内存、不写盘（启动还原 / 多文件拖入）
//   m_indicatorIndex  插入指示线当前下标，用于跳过无谓的布局重排
//
// 【函数一览】
//   ── 生命周期 ──────────────────────────────────────────────
//     MainWindow()            搭界面 → 还原配置里的分组 → 安排开机自启
//     ~MainWindow()           关闭前同步数据并落盘
//   ── 界面搭建 ──────────────────────────────────────────────
//     createToolbar()         构建顶部工具栏（标题 + 三个图标按钮）
//     addGroupPanel()         新增一块分组面板，并接好它的全部信号
//   ── 槽：分组 ──────────────────────────────────────────────
//     onNewGroup()            新建空分组
//     onGroupAddFiles()       点分组「+」→ 弹新增窗 → 加一行
//     onGroupDelete()         删除分组
//     onGroupLaunch()         启动整组
//     onGroupLaunchFile()     启动某一行
//     onGroupEditFile()       双击某行 → 弹编辑窗
//     onAnyChanged()          任何改动 → 同步 + 保存（批量操作时暂缓写盘）
//     syncGroupsToConfig()    按「布局顺序」把界面数据收集进 ConfigStore
//   ── 槽：其它 ──────────────────────────────────────────────
//     onSettings()            打开设置弹窗
//     onStatus()              打开后台记录弹窗
//   ── 拖放：拖入文件 ────────────────────────────────────────
//     dragEnterEvent()        只接受「文件」拖入
//     dropEvent()             松手 → 每个文件各建一个分组
//     createGroupsFromPaths() 批量建分组（内部用 batch 合并写盘）
//   ── 拖放：分组换位置 ──────────────────────────────────────
//     eventFilter()           拦截 m_groupHost 的拖拽事件，分流两种拖拽
//     insertionIndexAt()      由纵坐标算出应插入到第几个分组之前
//     updateDropIndicator()   把蓝色插入线移到目标位置
//     hideDropIndicator()     隐藏并摘除插入线
//     performGroupReorder()   松手时真正调整分组顺序
// =============================================================================

#include "mainwindow.h"
#include "groupwidget.h"
#include "fileitemwidget.h"
#include "settingsdialog.h"
#include "filedialog.h"
#include "logdialog.h"

#include "core/configstore.h"
#include "core/logger.h"
#include "core/launcher.h"
#include "core/iconpainter.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

// 工具栏图标统一颜色（深蓝灰）
static const QColor kIconColor(0x55, 0x5f, 0x6b);

// 「拖动分组」用的 MIME 类型：只构造一次。
// 拖拽过程中 DragMove 会高频触发，这里避免每次都新建 QString。
static const QString kGroupMime = QString::fromLatin1(TINYOPEN_GROUP_MIME);

// 构造：创建核心模块 → 搭建界面（工具栏 + 可滚动分组列表）→ 还原配置里的分组 → 执行开机自启
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Tinyopen"));
    resize(400, 560);            // 默认窗口尺寸
    setMinimumSize(300, 400);    // 最小窗口尺寸
    setAcceptDrops(true);        // 允许把文件拖进窗口

    // 创建三个核心模块：配置、日志、启动器
    m_config   = new ConfigStore(this);
    m_logger   = new Logger(this);
    m_launcher = new Launcher(m_logger, m_config, this);

    // 中央区域：浅灰底 + 无边框滚动条样式
    auto *central = new QWidget(this);
    central->setObjectName("central");
    central->setStyleSheet(
        "QWidget#central{background:#f7f8fa;}"
        "QScrollArea{border:none;background:transparent;}"
        "QScrollBar:vertical{background:transparent;width:8px;}"
        "QScrollBar::handle:vertical{background:#d3d8de;border-radius:4px;min-height:30px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}");
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---------- 顶栏 ----------
    root->addWidget(createToolbar());

    // ---------- 分组列表区（滚动） ----------
    m_scroll = new QScrollArea(central);
    m_scroll->setWidgetResizable(true);

    m_groupHost = new QWidget(m_scroll);
    m_groupHost->setObjectName("groupHost");
    m_groupHost->setStyleSheet("QWidget#groupHost{background:transparent;}");
    // 让分组容器接收拖拽，用于「拖动分组换位置」和「拖文件到空白处新建分组」
    m_groupHost->setAcceptDrops(true);
    m_groupHost->installEventFilter(this);

    m_groupLayout = new QVBoxLayout(m_groupHost);
    m_groupLayout->setContentsMargins(16, 14, 16, 16);
    m_groupLayout->setSpacing(12);
    m_groupLayout->addStretch();   // 末尾拉伸：让分组从顶部开始排列

    // 拖动分组时显示的「插入指示线」（一条蓝色横线，平时隐藏、不占布局）
    m_dropIndicator = new QFrame(m_groupHost);
    m_dropIndicator->setFixedHeight(3);
    m_dropIndicator->setStyleSheet("background:#2f7dd1;border-radius:1px;");
    m_dropIndicator->hide();

    m_scroll->setWidget(m_groupHost);
    root->addWidget(m_scroll, 1);

    // ---------- 载入已保存的分组（连同文件条目一起还原） ----------
    // 1) 先取一份快照：addGroupPanel 会回调 onAnyChanged 改动配置里的列表，
    //    直接遍历 m_config->groups() 不安全；
    // 2) 用 batch 把「每组各存一次盘」合并成最后统一存一次（原来 4 组要写 4 次文件）。
    const QList<GroupEntry> savedGroups = m_config->groups();
    ++m_batchDepth;
    for (const GroupEntry &g : savedGroups)
        addGroupPanel(g.name, {}, &g);   // 直接把整个分组数据交给面板还原
    --m_batchDepth;
    m_config->save();

    // ---------- 开机自启：还原完分组后，依次打开所有勾选了「自动打开」的分组 ----------
    // 用 QTimer 排到事件循环之后，确保窗口先显示出来再启动文件
    QTimer::singleShot(0, this, [this] {
        for (GroupWidget *g : findChildren<GroupWidget *>()) {
            if (g->autoLaunch())
                m_launcher->launchGroup(g->entry());
        }
    });
}

MainWindow::~MainWindow()
{
    // 关闭前把界面上的分组数据同步到配置并落盘
    syncGroupsToConfig();
    m_config->save();
}

// 构建顶部工具栏：左侧标题，右侧「新建分组 / 设置 / 状况」三个图标按钮
QWidget *MainWindow::createToolbar()
{
    auto *bar = new QWidget(this);
    bar->setObjectName("toolbar");
    bar->setFixedHeight(48);
    bar->setStyleSheet("QWidget#toolbar{background:#ffffff;border-bottom:1px solid #e6e9ee;}");

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(4);

    // 左侧标题
    auto *title = new QLabel(QStringLiteral("Tinyopen"), bar);
    title->setStyleSheet("font-size:14px;font-weight:600;color:#22303f;");
    layout->addWidget(title);
    layout->addStretch();

    // 右侧图标按钮：加号 / 齿轮 / 状态（用「图标类型 + 点击槽」的表格批量创建）
    struct BtnDef { const char *tip; IconPainter::IconType type; void (MainWindow::*slot)(); };
    const BtnDef defs[] = {
        { "新建分组", IconPainter::Plus,   &MainWindow::onNewGroup },
        { "设置",     IconPainter::Gear,   &MainWindow::onSettings },
        { "状况",     IconPainter::Status, &MainWindow::onStatus   },
    };
    for (const BtnDef &d : defs) {
        auto *btn = new QToolButton(bar);
        btn->setToolTip(QString::fromUtf8(d.tip));
        btn->setAutoRaise(true);            // 平时无边框，悬停才高亮
        btn->setFixedSize(32, 32);
        btn->setIcon(IconPainter::make(d.type, kIconColor, 20));
        btn->setIconSize(QSize(20, 20));
        btn->setStyleSheet(
            "QToolButton{border-radius:6px;}"
            "QToolButton:hover{background:#eef3fa;}");
        connect(btn, &QToolButton::clicked, this, d.slot);
        layout->addWidget(btn);
    }

    return bar;
}

// 向界面添加一个分组面板，并用信号把面板的请求接到主窗口的槽
void MainWindow::addGroupPanel(const QString &name, const QStringList &files,
                               const GroupEntry *savedGroup)
{
    GroupEntry e;
    e.name = name;
    auto *panel = new GroupWidget(e, m_groupHost);

    // 面板 → 主窗口 的请求信号连接
    connect(panel, &GroupWidget::requestDelete, this, &MainWindow::onGroupDelete);
    connect(panel, &GroupWidget::requestLaunch, this, &MainWindow::onGroupLaunch);
    connect(panel, &GroupWidget::requestLaunchFile, this, &MainWindow::onGroupLaunchFile);
    connect(panel, &GroupWidget::requestEditFile, this, &MainWindow::onGroupEditFile);
    connect(panel, &GroupWidget::requestAddFiles, this, &MainWindow::onGroupAddFiles);
    connect(panel, &GroupWidget::changed, this, &MainWindow::onAnyChanged);
    // 拖动分组换位置：开始拖动时记住它，结束时清空并隐藏指示线
    connect(panel, &GroupWidget::dragStarted, this, [this](GroupWidget *g) { m_dragGroup = g; });
    connect(panel, &GroupWidget::dragFinished, this, [this] {
        m_dragGroup = nullptr;
        hideDropIndicator();
    });

    // 插到末尾 stretch 之前（保证分组都在顶部）
    m_groupLayout->insertWidget(m_groupLayout->count() - 1, panel);

    // 还原完整分组数据（分组名 + 文件条目）
    if (savedGroup)
        panel->loadGroup(*savedGroup);
    else if (!files.isEmpty())
        panel->addFiles(files);

    onAnyChanged();
}

// 新建一个空分组（默认名「新分组」，加入首个文件后会自动改名）
void MainWindow::onNewGroup()
{
    addGroupPanel(QStringLiteral("新分组"));
}

// 点分组「添加」按钮 → 弹出「文件设置」窗（新增模式）
void MainWindow::onGroupAddFiles(GroupWidget *g)
{
    // 直接弹出「文件设置」窗（新增模式），手动填写 / 粘贴路径添加，不打开系统资源管理器
    FileDialog dlg(FileEntry(), this, /*isNew=*/true);
    if (dlg.exec() == QDialog::Accepted) {
        const FileEntry e = dlg.result();
        if (e.path.trimmed().isEmpty())
            return;   // 路径为空则不添加
        g->addFile(e);
    }
}

// 删除分组
void MainWindow::onGroupDelete(GroupWidget *g)
{
    m_groupLayout->removeWidget(g);
    g->deleteLater();     // 延迟销毁，避免在信号处理中立即析构
    onAnyChanged();
}

// 启动整个分组
void MainWindow::onGroupLaunch(GroupWidget *g)
{
    const GroupEntry e = g->entry();
    m_launcher->launchGroup(e);
}

// 启动单个文件（点该行的启动按钮）
void MainWindow::onGroupLaunchFile(GroupWidget *g, int row)
{
    if (row < 0)
        return;
    const FileEntry e = g->fileAt(row);
    m_launcher->launch(e, g->groupName());
}

// 双击文件行 → 编辑弹窗（显示名称 / 路径 / 启动参数）
void MainWindow::onGroupEditFile(GroupWidget *g, int row)
{
    if (row < 0)
        return;
    FileDialog dlg(g->fileAt(row), this);
    if (dlg.exec() == QDialog::Accepted)
        g->setFileAt(row, dlg.result());
}

// 任何数据变化 → 同步到 ConfigStore 并保存。
// 批量操作期间（m_batchDepth > 0）只同步内存，不写盘，由批量结束处统一保存一次。
void MainWindow::onAnyChanged()
{
    syncGroupsToConfig();
    if (m_batchDepth == 0)
        m_config->save();
}

// 把界面上所有 GroupWidget 的数据收集成一个列表，存入 ConfigStore。
// 注意：必须按「布局顺序」收集（而不是 findChildren 的创建顺序），
//       否则拖动换位置后的新顺序不会被保存。
void MainWindow::syncGroupsToConfig()
{
    QList<GroupEntry> list;
    list.reserve(m_groupLayout->count());   // 预分配，避免反复扩容
    for (int i = 0; i < m_groupLayout->count(); ++i) {
        auto *g = qobject_cast<GroupWidget *>(m_groupLayout->itemAt(i)->widget());
        if (g)
            list.append(g->entry());
    }
    m_config->setGroups(list);
}

// 打开设置弹窗（启动间隔）
void MainWindow::onSettings()
{
    SettingsDialog dlg(m_config->delayMs(), this);
    if (dlg.exec() == QDialog::Accepted)
        m_config->setDelayMs(dlg.delayMs());
}

// 打开后台记录弹窗
void MainWindow::onStatus()
{
    LogDialog dlg(m_logger->toDisplayText(), this);
    dlg.exec();
}

// ---------- 拖放到窗口（非分组区域）→ 新建分组，分组名 = 文件名 ----------
// 拖入窗口：只接受「文件（urls）」的拖拽
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

// 松手：把拖入的本地文件各自新建为一个分组
void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return;

    // 只取本地文件（忽略非本地 URL）
    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile())
            paths << url.toLocalFile();
    }
    if (paths.isEmpty())
        return;

    createGroupsFromPaths(paths);
    event->acceptProposedAction();
}

// 每个路径新建一个分组，分组名取文件名。
// 一次拖入多个文件时用 batch 合并写盘，避免「每建一组写一次配置」。
void MainWindow::createGroupsFromPaths(const QStringList &paths)
{
    if (paths.isEmpty())
        return;

    ++m_batchDepth;
    for (const QString &p : paths) {
        const QString base = QFileInfo(p).fileName();
        addGroupPanel(base.isEmpty() ? QStringLiteral("新分组") : base, {p});
    }
    --m_batchDepth;
    m_config->save();
}

// =============================================================================
// 拖动分组换位置：拦截分组容器上的拖拽事件
// -----------------------------------------------------------------------------
// 该容器同时要接收两种拖拽，用 MIME 类型区分：
//   TINYOPEN_GROUP_MIME → 移动分组：显示插入指示线，松手后调整顺序
//   含 urls             → 拖入文件：空白处新建分组（原「拖到空白处」功能）
// 拖到「某个分组身上」的文件由 GroupWidget 自己处理（加到该分组），不会到这里。
// =============================================================================
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_groupHost)
        return QMainWindow::eventFilter(obj, event);

    switch (event->type()) {
    case QEvent::DragEnter:
    case QEvent::DragMove: {
        auto *e = static_cast<QDropEvent *>(event);
        if (e->mimeData()->hasFormat(kGroupMime)) {
            // 移动分组：接受并更新插入指示线位置
            updateDropIndicator(e->position().toPoint());
            e->acceptProposedAction();
            return true;
        }
        if (e->mimeData()->hasUrls()) {
            // 拖入文件：接受，等松手时在空白处新建分组
            e->acceptProposedAction();
            return true;
        }
        return false;
    }
    case QEvent::DragLeave:
        hideDropIndicator();
        return false;
    case QEvent::Drop: {
        auto *e = static_cast<QDropEvent *>(event);
        if (e->mimeData()->hasFormat(kGroupMime)) {
            performGroupReorder(e->position().toPoint());
            hideDropIndicator();
            e->acceptProposedAction();
            return true;
        }
        if (e->mimeData()->hasUrls()) {
            QStringList paths;
            for (const QUrl &u : e->mimeData()->urls())
                if (u.isLocalFile())
                    paths << u.toLocalFile();
            if (!paths.isEmpty())
                createGroupsFromPaths(paths);
            e->acceptProposedAction();
            return true;
        }
        return false;
    }
    default:
        break;
    }
    return QMainWindow::eventFilter(obj, event);
}

// 由落点的纵坐标算出「应插入到第几个分组之前」。
// 返回 0..N：0 = 插到最前面，N = 插到最后面。
int MainWindow::insertionIndexAt(int y) const
{
    int idx = 0;   // 已经数过的分组个数
    for (int i = 0; i < m_groupLayout->count(); ++i) {
        auto *w = qobject_cast<GroupWidget *>(m_groupLayout->itemAt(i)->widget());
        if (!w)
            continue;   // 跳过 stretch / 指示线
        if (y < w->geometry().center().y())
            return idx; // 落在该分组上半部分 → 插到它前面
        ++idx;
    }
    return idx;         // 落到最后
}

// 把插入指示线移动到对应位置。
// DragMove 是高频事件，位置没变时直接返回，避免反复 remove/insert 造成布局抖动。
void MainWindow::updateDropIndicator(const QPoint &posInHost)
{
    // insertionIndexAt 只统计分组、会跳过指示线本身，所以「先算再摘」是安全的
    const int idx = insertionIndexAt(posInHost.y());
    if (idx == m_indicatorIndex && m_dropIndicator->isVisible())
        return;

    m_indicatorIndex = idx;
    m_groupLayout->removeWidget(m_dropIndicator);
    m_groupLayout->insertWidget(idx, m_dropIndicator);
    m_dropIndicator->show();
}

// 隐藏并摘除插入指示线
void MainWindow::hideDropIndicator()
{
    if (!m_dropIndicator)
        return;
    m_indicatorIndex = -1;
    m_groupLayout->removeWidget(m_dropIndicator);
    m_dropIndicator->hide();
}

// 松手时真正调整分组顺序
void MainWindow::performGroupReorder(const QPoint &posInHost)
{
    if (!m_dragGroup)
        return;

    // 先把指示线从布局里摘掉，让下标只对应分组本身
    m_groupLayout->removeWidget(m_dropIndicator);

    const int cur = m_groupLayout->indexOf(m_dragGroup);
    if (cur < 0)
        return;

    int target = insertionIndexAt(posInHost.y());   // 目标插入位（基于当前顺序）
    if (cur < target)
        --target;                                    // 自己会被移除，后面元素前移一位

    if (target == cur)
        return;                                      // 位置没变，不用动

    m_groupLayout->removeWidget(m_dragGroup);
    m_groupLayout->insertWidget(target, m_dragGroup);
    onAnyChanged();                                  // 新顺序写入配置
}

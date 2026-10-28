// =============================================================================
// groupwidget.h — 分组面板（一块「分组」卡片）
// =============================================================================
//
// 【界面结构】
//   ┌──────────────────────────────────────────────────────┐
//   │ 浅蓝标题栏： 分组名（点击可改名）   [🔌][＋][▶][🗑]   │
//   ├──────────────────────────────────────────────────────┤
//   │  FileItemWidget  第 1 行                             │
//   │  FileItemWidget  第 2 行                             │
//   │  …（没有文件时显示「把文件拖到这里…」的灰色提示）      │
//   └──────────────────────────────────────────────────────┘
//   标题栏右侧四个按钮依次是：🔌 开机自启 / ＋ 添加文件 / ▶ 启动整组 / 🗑 删除分组
//
// 【职责】
//   * 展示并维护「一个分组」的全部数据（分组名、文件行、开机自启标记）；
//   * 改名：点击标题栏上的分组名 → 原地变成输入框；
//   * 添加 / 删除文件行；编辑请求转发给主窗口（弹窗由主窗口负责）；
//   * 接收从资源管理器拖进来的文件；
//   * 按住标题栏拖动 → 发起「移动整个分组」的拖拽；
//   * 任何内容变化 emit changed，由主窗口统一保存。
//
// 【类结构】
//   公有    ：entry() / groupName() / fileAt() / setFileAt() / autoLaunch() / setAutoLaunch()
//   公有槽  ：addFiles() / addFile() / loadGroup()      —— 供主窗口或拖放调用
//   信号    ：request*（请求主窗口做事）/ dragStarted、dragFinished（拖动）/ changed（需保存）
//   受保护  ：dragEnterEvent、dropEvent（接收拖入文件）/ eventFilter（改名 + 拖动）
//   私有    ：rebuildRows / appendRow / updateEmptyHint / updateAutoButton /
//             updateMinimumHeight / startGroupDrag / rowIndex / maybeAutoRename
//   成员    ：m_entry（本组数据）、m_rows（行控件列表）、m_rowLayout（行布局）、
//             标题栏与四个按钮指针、m_headerPressPos（拖动起点）、
//             m_minHeightQueued（最小高度重算去重标志）
//
// 【关键约束】
//   1. 本类用样式表画背景/边框，构造时必须 setAttribute(WA_StyledBackground, true)；
//   2. 最小高度必须跟随内容高度（见 updateMinimumHeight 的详细说明），
//      否则内容超出视口时分组会被压扁，Qt 会把行间距压成负值 → 文件行重叠；
//   3. 「移动分组」用 TINYOPEN_GROUP_MIME 标记，与「拖入文件」区分开。
// =============================================================================

#ifndef GROUPWIDGET_H
#define GROUPWIDGET_H

#include <QWidget>
#include <QList>
#include <QString>
#include <QPoint>
#include "core/groupdata.h"

// 拖动「整个分组」换位置时使用的自定义 MIME 类型。
// 用它把「移动分组」和「拖文件进来」两种拖拽区分开：
//   有本类型   → 移动分组（在分组列表里换顺序）
//   只有 urls  → 拖入文件（加到分组里 / 空白处新建分组）
#define TINYOPEN_GROUP_MIME "application/x-tinyopen-group"

class QLabel;
class QLineEdit;
class QVBoxLayout;
class QToolButton;
class FileItemWidget;

// 一个分组面板
class GroupWidget : public QWidget
{
    Q_OBJECT
public:
    // entry 为初始数据（新建时通常只有名字）；构造时按它搭好整个面板
    explicit GroupWidget(const GroupEntry &entry, QWidget *parent = nullptr);

    GroupEntry entry() const;                      // 取出当前完整数据
    QString groupName() const { return m_entry.name; }   // 只取分组名

    FileEntry fileAt(int index) const;             // 读第 index 行
    void setFileAt(int index, const FileEntry &entry);  // 改第 index 行

    // 是否在软件启动时自动打开本分组
    bool autoLaunch() const { return m_entry.autoLaunch; }
    void setAutoLaunch(bool on);                   // 设置开机自启（会触发保存）

public slots:
    // 拖动放入：新增一行文件行
    void addFiles(const QStringList &paths);

    // 手动新增一条文件记录（供「添加」弹窗使用）
    void addFile(const FileEntry &entry);

    // 用完整数据替换当前分组内容（用于从配置还原）
    void loadGroup(const GroupEntry &entry);

signals:
    void requestDelete(GroupWidget *self);               // 请求删除本分组
    void requestLaunch(GroupWidget *self);               // 请求启动整个分组
    void requestLaunchFile(GroupWidget *self, int row);  // 请求启动单个文件
    void requestEditFile(GroupWidget *self, int row);    // 请求编辑单个文件
    void requestAddFiles(GroupWidget *self);             // 点击「添加」按钮
    void dragStarted(GroupWidget *self);                 // 开始用标题栏拖动本分组
    void dragFinished();                                 // 分组拖动结束（松手）
    void changed();                                      // 任何内容变化，触发保存

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;   // 拖入：有文件 URL 才接受
    void dropEvent(QDropEvent *event) override;             // 放下：把文件加进本分组
    bool eventFilter(QObject *obj, QEvent *event) override;   // 点击分组名改名 / 标题栏拖动

private slots:
    void onRenameClicked();     // 显示行内编辑框改名
    void onNameEdited();        // 改名完成，落库

private:
    void rebuildRows();                         // 清空并按 m_entry 重建所有文件行
    void appendRow(const FileEntry &entry);     // 追加一行文件行
    void updateEmptyHint();                     // 根据是否有文件显示/隐藏空提示
    void updateAutoButton();                    // 刷新开机自启按钮状态
    void updateMinimumHeight();                 // 让最小高度跟上内容高度（防止被压缩重叠）
    void startGroupDrag();                      // 发起「拖动整个分组」的 QDrag
    int rowIndex(const FileItemWidget *row) const;   // 由行控件反查其在列表中的下标

    // 默认新分组名；若分组仍是此名字，则在其加入第一个文件时自动改名
    static QString defaultGroupName() { return QStringLiteral("新分组"); }
    // 若分组仍叫「新分组」且此前没有任何文件，则用首个文件名作为分组名
    void maybeAutoRename(const FileEntry &firstAdded);

    GroupEntry m_entry;                 // 本分组的全部数据

    QWidget     *m_headerBar = nullptr; // 标题栏（按住它可拖动整个分组）
    QPoint       m_headerPressPos;      // 标题栏按下时的坐标（用于判断是否开始拖动）
    bool         m_minHeightQueued = false;  // 「最小高度重算」是否已排队（避免重复排定时器）

    QLabel      *m_nameLabel = nullptr; // 分组名（展示态）
    QLineEdit   *m_nameEdit  = nullptr; // 分组名（编辑态）
    QToolButton *m_addBtn    = nullptr; // 添加文件
    QToolButton *m_launchBtn = nullptr; // 启动分组
    QToolButton *m_deleteBtn = nullptr; // 删除分组
    QToolButton *m_autoBtn   = nullptr; // 开机自启开关

    QVBoxLayout             *m_rowLayout = nullptr;  // 文件行的纵向布局
    QLabel                  *m_emptyHint = nullptr;  // 「拖文件到这里」提示
    QList<FileItemWidget *>  m_rows;                 // 所有文件行控件
};

#endif // GROUPWIDGET_H

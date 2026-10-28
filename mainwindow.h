// =============================================================================
// mainwindow.h — 主窗口（整个程序的调度中枢）
// =============================================================================
//
// 【职责】
//   顶部是工具栏（标题 + 新建分组 / 设置 / 状况 三个图标按钮），
//   下方是可滚动的分组列表。具体负责：
//     * 从 ConfigStore 还原分组，并把分组变化写回 ConfigStore；
//     * 把 GroupWidget 的各种请求（增删文件、启动、编辑）转给 Launcher 或弹窗；
//     * 处理「拖文件到空白处 → 新建分组」；
//     * 处理「拖动分组标题栏 → 在列表里换位置」（含蓝色插入指示线）；
//     * 启动后按每个分组的 autoLaunch 标记执行开机自启。
//
// 【类结构】
//   公有   ：构造 / 析构
//   受保护 ：dragEnterEvent / dropEvent / eventFilter —— 都是 Qt 回调，不主动调用
//   私有槽 ：全部业务处理（函数清单见 mainwindow.cpp 顶部）
//   私有成员：
//       m_config / m_logger / m_launcher   三个 core 模块
//       m_groupLayout / m_groupHost / m_scroll  界面容器
//       m_dragGroup / m_dropIndicator / m_indicatorIndex  拖动换位状态
//       m_batchDepth   批量操作计数（>0 时不写盘）
//
// 【信号接线】（全部在 addGroupPanel() 里一次性连好）
//   GroupWidget::requestDelete     → onGroupDelete      删除分组
//   GroupWidget::requestLaunch     → onGroupLaunch      启动整组
//   GroupWidget::requestLaunchFile → onGroupLaunchFile  启动某一行
//   GroupWidget::requestEditFile   → onGroupEditFile    编辑某一行
//   GroupWidget::requestAddFiles   → onGroupAddFiles    「+」添加
//   GroupWidget::changed           → onAnyChanged       任何改动 → 保存
//   GroupWidget::dragStarted       → 记录 m_dragGroup
//   GroupWidget::dragFinished      → 清空 m_dragGroup 并隐藏指示线
// =============================================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include "core/groupdata.h"

class QVBoxLayout;
class QWidget;
class QScrollArea;
class QFrame;

class ConfigStore;
class Logger;
class Launcher;
class GroupWidget;

// 主窗口
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);   // 构造：建界面 + 还原分组 + 开机自启
    ~MainWindow() override;   // 析构时把分组数据落盘

protected:
    // 拖放：把文件拖到窗口空白处 → 每个文件新建一个分组
    void dragEnterEvent(QDragEnterEvent *event) override;   // 拖入：接受文件
    void dropEvent(QDropEvent *event) override;             // 放下：按文件新建分组

    // 事件过滤器：拦截分组列表容器上的拖拽，用于「拖动分组换位置」
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onNewGroup();                                  // 新建空分组
    void onSettings();                                  // 打开设置弹窗
    void onStatus();                                    // 打开后台记录弹窗

    void onGroupDelete(GroupWidget *g);                 // 删除分组
    void onGroupLaunch(GroupWidget *g);                 // 启动整个分组
    void onGroupLaunchFile(GroupWidget *g, int row);    // 启动分组内某一行
    void onGroupEditFile(GroupWidget *g, int row);      // 编辑某一行
    void onGroupAddFiles(GroupWidget *g);               // 点分组「添加」→ 弹新增窗
    void onAnyChanged();                                // 任何数据变化 → 同步 + 保存

private:
    QWidget *createToolbar();                           // 构建顶部工具栏
    // 向界面添加一个分组面板。
    //   savedGroup 非空：直接用完整数据还原（从配置恢复时用）
    //   files 非空     ：用这些路径新建文件行（拖放新建分组时用）
    void addGroupPanel(const QString &name, const QStringList &files = {},
                       const GroupEntry *savedGroup = nullptr);
    void syncGroupsToConfig();                          // 收集界面分组数据到 ConfigStore（按界面顺序）
    void createGroupsFromPaths(const QStringList &paths);  // 每个路径新建一个分组

    // —— 拖动分组换位置相关 ——
    void updateDropIndicator(const QPoint &posInHost);  // 按落点移动插入指示线
    void hideDropIndicator();                           // 隐藏插入指示线
    int  insertionIndexAt(int y) const;                 // 由纵坐标算出应插入到第几个分组之前
    void performGroupReorder(const QPoint &posInHost);  // 松手时真正调整分组顺序

    ConfigStore *m_config = nullptr;    // 配置（含启动间隔、分组数据）
    Logger      *m_logger = nullptr;    // 后台记录器
    Launcher    *m_launcher = nullptr;  // 启动器

    QVBoxLayout *m_groupLayout = nullptr;  // 分组面板的纵向布局
    QWidget     *m_groupHost   = nullptr;  // 分组面板的容器
    QScrollArea *m_scroll      = nullptr;  // 滚动区域

    GroupWidget *m_dragGroup = nullptr;      // 当前正在被拖动的分组（拖动中才有值）
    QFrame      *m_dropIndicator = nullptr;  // 拖动时显示的插入指示线

    int m_batchDepth = 0;       // >0 表示正在批量操作：期间只同步内存、不写盘，结束后统一保存一次
    int m_indicatorIndex = -1;  // 插入指示线当前所在的下标（-1 表示未显示），用于跳过无谓的布局重排
};

#endif // MAINWINDOW_H

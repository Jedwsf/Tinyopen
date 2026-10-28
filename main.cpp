// =============================================================================
// Tinyopen — 程序入口 ＋ 全项目总览
// =============================================================================
//
// ■ 一、这个项目是什么
//   Tinyopen 是一个 Windows 桌面小工具：把常用的「文件 / 程序 / 网页」分组收纳，
//   之后一键批量打开。例如把「每天开工要开的几个软件」放进一个分组，点一下全开。
//     - 绿色便携：不需要安装，配置和日志都放在 exe 旁边，整个文件夹拷走即可迁移
//     - 版本 1.2.1（版本号统一定义在 settingsdialog.cpp 的 VERSION 宏）
//     - 适用平台：64 位 Windows 10 / 11
//
// ■ 二、技术栈与构建方式
//   - 语言      ：C++17
//   - 框架      ：Qt 6.8.2 Widgets（只用到 Core / Gui / Widgets 三个模块）
//   - 编译器    ：MinGW 13.1.0（64 位）
//   - 构建方式（二选一）：
//       1) 命令行（推荐）：bash scripts/build.sh
//          该脚本绕过 qmake，直接按 moc → 编译 → 链接 的顺序手动构建，
//          产物为 build/Tinyopen.exe。
//       2) Qt Creator：打开 Tinyopen.pro 构建（.pro 中已配置好 INCLUDEPATH）。
//   - 图标不使用任何图片资源，全部由 QPainter 在运行时手绘（core/iconpainter.cpp）。
//
// ■ 三、目录结构与每个文件的职责
//   Tinyopen/
//   ├── main.cpp                  ← 本文件：程序入口 + 这份全项目说明
//   ├── mainwindow.h/.cpp           主窗口：工具栏 + 可滚动分组列表；全局调度中枢
//   ├── groupwidget.h/.cpp          单个「分组面板」：标题栏 + 若干文件行
//   ├── fileitemwidget.h/.cpp       单个「文件行」：类型图标 + 名称 + 启动/删除按钮
//   ├── filedialog.h/.cpp           「添加 / 编辑文件」弹窗（名称、路径、启动参数）
//   ├── settingsdialog.h/.cpp       「设置」弹窗（启动间隔、版本号）
//   ├── logdialog.h/.cpp            「后台记录」弹窗（只读展示本次运行日志）
//   ├── core/
//   │   ├── groupdata.h             数据模型 FileEntry / GroupEntry + 网址识别工具
//   │   ├── configstore.h/.cpp      配置读写（config.json）
//   │   ├── logger.h/.cpp           后台记录（tinylog.txt）
//   │   ├── launcher.h/.cpp         真正执行「打开文件 / 打开网址」（ShellExecuteW）
//   │   └── iconpainter.h/.cpp      全部图标的手绘实现
//   ├── scripts/build.sh            手动构建脚本
//   ├── Tinyopen.pro                qmake 工程文件（供 Qt Creator 使用）
//   └── build/
//       ├── Tinyopen.exe            开发版程序（依赖同目录下的 dll）
//       └── Release/                自包含分发包（含全部依赖 + README.md）
//
// ■ 四、运行时架构（三层）
//   ┌──────────────────── 界面层 ────────────────────┐
//   │ MainWindow                                      │ 持有 QScrollArea，管理所有分组
//   │  ├─ GroupWidget × N                             │ 一个分组 = 一块面板
//   │  │    └─ FileItemWidget × M                     │ 面板里的每一行文件
//   │  └─ FileDialog / SettingsDialog / LogDialog     │ 三个模态弹窗
//   └────────────────────────┬────────────────────────┘
//                            │ 信号 / 槽
//   ┌────────────────────────┴────────────────────────┐
//   │ 逻辑层（core/）                                  │
//   │   ConfigStore  读写 config.json                  │
//   │   Logger       追加写 tinylog.txt                │
//   │   Launcher     调 ShellExecuteW 打开文件/网址     │
//   └────────────────────────┬────────────────────────┘
//                            │
//   ┌────────────────────────┴────────────────────────┐
//   │ 数据层（core/groupdata.h）                       │
//   │   FileEntry（一行） / GroupEntry（一组）          │
//   └─────────────────────────────────────────────────┘
//
// ■ 五、五条关键数据流
//   1) 启动流程
//        main() → MainWindow 构造
//          → ConfigStore 构造时 load() 读取 config.json
//          → 逐个 addGroupPanel() 建出 GroupWidget，再 loadGroup() 还原数据
//          → QTimer::singleShot(0) 中对勾选了「开机自启」的分组调 launchGroup()
//   2) 启动单个文件
//        FileItemWidget 的「▶」被点击 → emit launchRequested
//          → GroupWidget 转发 requestLaunchFile(this, row)
//          → MainWindow::onGroupLaunchFile → Launcher::launch() → ShellExecuteW
//   3) 编辑某一行
//        双击 FileItemWidget → emit editRequested
//          → MainWindow::onGroupEditFile 弹出 FileDialog
//          → 用户确定后 GroupWidget::setFileAt() → emit changed → 保存
//   4) 拖动分组换位置
//        按住标题栏拖动 → GroupWidget::startGroupDrag() 发起 QDrag（带自定义 MIME）
//          → MainWindow::eventFilter 拦截 m_groupHost 上的拖拽事件
//          → updateDropIndicator() 显示蓝色插入线
//          → 松手时 performGroupReorder() 调整布局顺序 → 保存
//   5) 保存流程
//        任何改动 → GroupWidget::changed → MainWindow::onAnyChanged()
//          → syncGroupsToConfig()（按「布局顺序」收集）
//          → ConfigStore::save() 写入 config.json
//
// ■ 六、数据模型与持久化
//   FileEntry  { displayName, path, arguments }        —— 一行文件/网址
//   GroupEntry { name, files[], autoLaunch }           —— 一个分组
//   config.json（程序所在目录）：
//     {
//       "delayMs": 200,
//       "groups": [
//         { "name": "…", "autoLaunch": false,
//           "files": [ { "displayName": "…", "path": "…", "arguments": "…" } ] }
//       ]
//     }
//   tinylog.txt（程序所在目录）：每行 "[时间] [分类] 正文"，只记录本次运行
//
// ■ 七、重要约定与易错点（改代码前务必先看这一节）
//   1. 自定义 QWidget 若用样式表画背景/边框，必须
//      setAttribute(Qt::WA_StyledBackground, true)，否则 QSS 的 background/border 不生效。
//   2. 包含 core 下的头文件统一写 "core/xxx.h"，
//      不要写 "../core/xxx.h"（根目录上一级没有 core，Qt Creator 会报找不到）。
//   3. 网址绝不能做 QDir::toNativeSeparators()，否则 "/" 会被替换成 "\"，网址就废了。
//   4. 列表项（分组）的最小高度必须跟随内容高度，不能写死。
//      否则内容超出视口时会被外层布局压扁，而 Qt 在空间不足时会把行间距压成负值，
//      最终表现为「文件行互相重叠」。
//   5. syncGroupsToConfig() 必须按「布局顺序」收集分组，不能用 findChildren
//      （findChildren 是创建顺序，拖动换位后不会反映新顺序）。
//   6. 「移动分组」与「拖入文件」是两种拖拽，靠自定义 MIME 类型区分：
//      TINYOPEN_GROUP_MIME（定义在 groupwidget.h）。
//
// ■ 八、本文件自己做什么
//   只做三件事：创建 QApplication → 设置全局字体 → 创建并显示 MainWindow。
// =============================================================================

#include "mainwindow.h"

#include <QApplication>
#include <QFont>

// 程序入口
//   1) 创建 Qt 应用对象（GUI 程序必须最先创建）；
//   2) 统一设置全局字体为微软雅黑 9pt，保证中文显示清晰、各控件字号一致；
//   3) 创建主窗口并进入事件循环（app.exec() 会阻塞到窗口关闭）。
int main(int argc, char *argv[])
{
    // 创建 Qt 应用对象（GUI 程序必须最先创建）
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tinyopen"));
    app.setOrganizationName(QStringLiteral("Tinyopen"));

    // 统一设置全局字体：微软雅黑，9 号字
    QFont f = app.font();
    f.setFamily(QStringLiteral("Microsoft YaHei"));
    f.setPointSize(9);
    app.setFont(f);

    // 创建主窗口，显示后进入 Qt 事件循环（阻塞直到窗口关闭）
    MainWindow w;
    w.show();
    return app.exec();
}

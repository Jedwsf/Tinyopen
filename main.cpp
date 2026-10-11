// =============================================================================
// Tinyopen — 程序入口
// -----------------------------------------------------------------------------
// 一个用于「把常用文件/网页分组，一键批量打开」的 Windows 桌面小工具。
// 技术栈：Qt 6.8.2 Widgets（C++17）+ MinGW 13.1.0。
//
// 本文件只做三件事：
//   1. 创建 QApplication 应用对象；
//   2. 设置全局字体（微软雅黑 9pt，中文显示更清晰）；
//   3. 创建并显示主窗口 MainWindow，进入事件循环。
// =============================================================================

#include "mainwindow.h"

#include <QApplication>
#include <QFont>

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

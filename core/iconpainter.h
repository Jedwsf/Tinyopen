// =============================================================================
// core/iconpainter.h — 图标绘制
// =============================================================================
//
// 【在项目中的位置】
//   被界面层调用：工具栏按钮、分组标题栏按钮、文件行的类型图标都来自这里。
//   只依赖 QtGui，不依赖任何界面类。
//
// 【设计思路】
//   本项目不使用任何图片资源（没有 .qrc，也没有 png / svg 文件）。
//   所有图标都在运行时用 QPainter 手绘，好处是：
//     - 体积小，没有外部资源依赖，打包不会漏文件
//     - 任意颜色、任意像素尺寸都能即时生成（高分屏也不糊）
//     - 风格天然统一（线性、圆角、约 1.7～2px 描边）
//
// 【本文件提供什么】
//   IconType 枚举：Plus / Gear / Status / Play / Trash / File / Folder /
//                  App / Music / Doc / Image / Power / Globe
//   make(type, color, px)  生成指定颜色与尺寸的 QIcon
//   typeForPath(path)      按「网址 / 目录 / 扩展名」推断该用哪个图标
// =============================================================================

#ifndef ICONPAINTER_H
#define ICONPAINTER_H

#include <QIcon>
#include <QColor>
#include <QString>

// 图标绘制命名空间
namespace IconPainter {

// 全部可用图标类型
enum IconType {
    Plus,        // 加号：新建分组
    Gear,        // 齿轮：设置
    Status,      // 状态：后台记录
    Play,        // 启动
    Trash,       // 删除
    File,        // 文件（通用）
    Folder,      // 文件夹
    App,         // 可执行程序
    Music,       // 音乐
    Doc,         // 文档
    Image,       // 图片
    Power,       // 开机自启（电源）
    Globe        // 网页链接
};

// 生成指定颜色、指定像素尺寸的图标
QIcon make(IconType type, const QColor &color, int px = 18);

// 根据文件路径（或网址）推断应使用的图标类型
IconType typeForPath(const QString &path);

} // namespace IconPainter

#endif // ICONPAINTER_H

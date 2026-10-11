// =============================================================================
// core/iconpainter.h — 图标绘制
// -----------------------------------------------------------------------------
// 本项目不使用任何图片资源，所有图标均由 QPainter 手工绘制，风格统一
// （线性、圆角、约 1.7~2px 描边）。好处：体积小、任意颜色/尺寸、无外部依赖。
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

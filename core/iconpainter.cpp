// =============================================================================
// core/iconpainter.cpp — 图标绘制实现
// =============================================================================
//
// 【统一约定】
//   每个图标一个 drawXxx 函数，签名统一为：
//       void drawXxx(QPainter &p, const QRectF &r, const QColor &c)
//   其中 r 是 (0,0,px,px) 的绘制区域，c 是描边/填充颜色。
//   所有坐标都按 r.width() / r.height() 的比例计算，因此任意尺寸都不会变形。
//
// 【绘制流程（make）】
//   建 px×px 透明 QPixmap → 开抗锯齿 → switch 分派到对应 drawXxx
//   → 包成 QIcon 返回。不读写任何图片文件。
//
// 【图标清单】
//   drawPlus    加号        —— 新建分组 / 添加文件
//   drawGear    齿轮        —— 设置
//   drawStatus  钟表        —— 后台记录（状况）
//   drawPlay    播放三角    —— 启动分组 / 启动文件
//   drawTrash   垃圾桶      —— 删除分组 / 删除文件
//   drawFile    白纸        —— 未知类型的通用文件
//   drawFolder  文件夹      —— 目录
//   drawApp     窗口+三角   —— exe / lnk / bat 等可执行程序
//   drawMusic   八分音符    —— 音频文件
//   drawDoc     纸+横线     —— 文本 / 办公文档
//   drawImage   相框+山      —— 图片
//   drawGlobe   地球        —— 网页链接
//   drawPower   电源符号    —— 开机自启开关
//
// 【复用的小工具】
//   paperPath()     生成「右上角带折角」的纸张路径（drawFile / drawDoc 共用）
//   drawFileBase()  画纸张外形 + 折角线，可选浅色填充
//   typeForPath()   按 网址 → 目录 → 扩展名 的顺序推断图标类型
// =============================================================================

#include "iconpainter.h"
#include "groupdata.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QFileInfo>

namespace IconPainter {

// 加号：用于「新建分组 / 添加文件」按钮。
// 画法：以图标中心为原点，画一横一竖两条等长线段（半长 = 宽度 34%），
//       使用圆头画笔（RoundCap）让线端更柔和。
static void drawPlus(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    const QPointF ctr = r.center();
    const qreal h = r.width() * 0.34;
    p.drawLine(QPointF(ctr.x() - h, ctr.y()), QPointF(ctr.x() + h, ctr.y()));
    p.drawLine(QPointF(ctr.x(), ctr.y() - h), QPointF(ctr.x(), ctr.y() + h));
}

// 齿轮：用于「设置」按钮。
// 画法：以中心为圆心，先沿圆周均匀画 8 条短齿（从半径 0.72*outer 到 outer），
//       再画外圈（半径 0.72*outer）与内圈（半径 inner）两个圆，构成齿轮轮廓。
static void drawGear(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QPointF ctr = r.center();
    const qreal outer = r.width() * 0.44;
    const qreal inner = r.width() * 0.20;

    // 齿
    const int teeth = 8;
    for (int i = 0; i < teeth; ++i) {
        const qreal a = (M_PI * 2.0 / teeth) * i;
        const QPointF p1(ctr.x() + qCos(a) * outer * 0.72, ctr.y() + qSin(a) * outer * 0.72);
        const QPointF p2(ctr.x() + qCos(a) * outer,      ctr.y() + qSin(a) * outer);
        p.drawLine(p1, p2);
    }
    // 外圈 + 内圈
    p.drawEllipse(ctr, outer * 0.72, outer * 0.72);
    p.drawEllipse(ctr, inner, inner);
}

// 状态（钟表）：用于「后台记录」按钮。
// 画法：一个圆环（表盘）+ 从圆心出发的两根表针（一根朝上、一根朝右下，模拟时针分针）。
static void drawStatus(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QPointF ctr = r.center();
    const qreal rad = r.width() * 0.38;

    // 外圈
    p.drawEllipse(ctr, rad, rad);
    // 表针
    p.drawLine(ctr, QPointF(ctr.x(), ctr.y() - rad * 0.62));
    p.drawLine(ctr, QPointF(ctr.x() + rad * 0.46, ctr.y() + rad * 0.18));
}

// 启动：用于「启动分组 / 启动文件」按钮。
// 画法：一个指向右侧的实心三角形（只填充、不描边），是通用的「播放」符号。
static void drawPlay(QPainter &p, const QRectF &r, const QColor &c)
{
    QPainterPath path;
    const QPointF ctr = r.center();
    const qreal h = r.width() * 0.30;
    path.moveTo(ctr.x() - h * 0.65, ctr.y() - h);
    path.lineTo(ctr.x() + h * 0.95, ctr.y());
    path.lineTo(ctr.x() - h * 0.65, ctr.y() + h);
    path.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawPath(path);
}

// 删除：用于「删除分组 / 删除文件」按钮。
// 画法：垃圾桶 = 盖板（横线）+ 提手（上方小方框）+ 梯形桶身 + 桶内两条竖线，
//       全部用线段与路径勾勒（不填充）。
static void drawTrash(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal w = r.width() * 0.56;
    const qreal x = r.center().x() - w / 2.0;
    const qreal top = r.center().y() - r.height() * 0.26;
    const qreal bottom = r.center().y() + r.height() * 0.34;

    // 盖
    p.drawLine(QPointF(x - w * 0.16, top), QPointF(x + w + w * 0.16, top));
    // 提手
    p.drawLine(QPointF(x + w * 0.28, top), QPointF(x + w * 0.28, top - r.height() * 0.12));
    p.drawLine(QPointF(x + w * 0.72, top), QPointF(x + w * 0.72, top - r.height() * 0.12));
    p.drawLine(QPointF(x + w * 0.28, top - r.height() * 0.12),
               QPointF(x + w * 0.72, top - r.height() * 0.12));
    // 桶身
    QPainterPath body;
    body.moveTo(x + w * 0.06, top);
    body.lineTo(x + w * 0.16, bottom);
    body.lineTo(x + w * 0.84, bottom);
    body.lineTo(x + w * 0.94, top);
    p.drawPath(body);
    // 两条竖线
    p.drawLine(QPointF(x + w * 0.36, top + r.height() * 0.10),
               QPointF(x + w * 0.38, bottom - r.height() * 0.06));
    p.drawLine(QPointF(x + w * 0.64, top + r.height() * 0.10),
               QPointF(x + w * 0.62, bottom - r.height() * 0.06));
}

// 纸张外形路径（供「通用文件 / 文档」类图标复用）。
// 画法：一个矩形，右上角切掉一块形成折角；fold 控制折角大小（相对纸张短边的比例）。
// 返回：以 r 的中心为基准居中的 QPainterPath。
static QPainterPath paperPath(const QRectF &r, qreal fold)
{
    const qreal w = r.width() * 0.56;
    const qreal h = r.height() * 0.66;
    const qreal x = r.center().x() - w / 2.0;
    const qreal y = r.center().y() - h / 2.0;
    const qreal f = qMin(w, h) * fold;

    QPainterPath path;
    path.moveTo(x, y);
    path.lineTo(x + w - f, y);
    path.lineTo(x + w, y + f);
    path.lineTo(x + w, y + h);
    path.lineTo(x, y + h);
    path.closeSubpath();
    return path;
}

// 纸张基础绘制：描边 + 折角线。
// withAccent=true 时用 accent 色的半透明（alpha 40）填充纸张内部，否则只描边不填充。
static void drawFileBase(QPainter &p, const QRectF &r, const QColor &c, const QColor &accent,
                         bool withAccent)
{
    QPen pen(c, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(withAccent ? QColor(accent.red(), accent.green(), accent.blue(), 40) : Qt::NoBrush);
    const QPainterPath path = paperPath(r, 0.30);
    p.drawPath(path);

    // 折角
    const QRectF br = path.boundingRect();
    const qreal f = qMin(br.width(), br.height()) * 0.30;
    p.setBrush(Qt::NoBrush);
    p.drawLine(QPointF(br.right() - f, br.top()), QPointF(br.right() - f, br.top() + f));
    p.drawLine(QPointF(br.right() - f, br.top() + f), QPointF(br.right(), br.top() + f));
}

// 通用文件：一张白纸（不填充），用于无法归类的文件类型。
static void drawFile(QPainter &p, const QRectF &r, const QColor &c)
{
    drawFileBase(p, r, c, c, false);
}

// 文件夹：用于目录。
// 画法：先画一条带「标签凸起」的轮廓路径（上边左段抬高，形成文件夹标签），
//       再用半透明色填充内部。
static void drawFolder(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal w = r.width() * 0.68;
    const qreal h = r.height() * 0.50;
    const qreal x = r.center().x() - w / 2.0;
    const qreal y = r.center().y() - h / 2.0;

    QPainterPath path;
    path.moveTo(x, y + h);
    path.lineTo(x, y + h * 0.12);
    path.lineTo(x + w * 0.40, y + h * 0.12);
    path.lineTo(x + w * 0.52, y + h * 0.36);
    path.lineTo(x + w, y + h * 0.36);
    path.lineTo(x + w, y + h);
    path.closeSubpath();
    p.setBrush(QColor(c.red(), c.green(), c.blue(), 46));
    p.drawPath(path);
}

// 可执行程序：用于 exe/lnk/bat 等。
// 画法：一个圆角窗口（上 26% 处画一条顶栏分隔线）+ 窗口内部的播放三角，
//       表示「可以运行的程序」。
static void drawApp(QPainter &p, const QRectF &r, const QColor &c)
{
    // 圆角窗口 + 内部播放三角
    QPen pen(c, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(QColor(c.red(), c.green(), c.blue(), 40));
    const qreal w = r.width() * 0.66;
    const qreal h = r.height() * 0.60;
    QRectF box(r.center().x() - w / 2.0, r.center().y() - h / 2.0, w, h);
    p.drawRoundedRect(box, 3.0, 3.0);

    // 顶栏
    p.setBrush(Qt::NoBrush);
    p.drawLine(QPointF(box.left(), box.top() + h * 0.26),
               QPointF(box.right(), box.top() + h * 0.26));

    // 播放三角
    QPainterPath tri;
    const QPointF ctr = QPointF(box.center().x(), box.top() + h * 0.62);
    const qreal t = h * 0.20;
    tri.moveTo(ctr.x() - t * 0.6, ctr.y() - t);
    tri.lineTo(ctr.x() + t * 0.9, ctr.y());
    tri.lineTo(ctr.x() - t * 0.6, ctr.y() + t);
    tri.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawPath(tri);
}

// 音乐：用于音频文件。
// 画法：一根竖直符杆 + 杆顶的小旗 + 左下角的实心椭圆符头，构成一个八分音符。
static void drawMusic(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal x = r.center().x() - r.width() * 0.10;
    const qreal top = r.center().y() - r.height() * 0.30;
    const qreal bottom = r.center().y() + r.height() * 0.14;

    // 符杆
    p.drawLine(QPointF(x, top), QPointF(x, bottom));
    p.drawLine(QPointF(x, top), QPointF(x + r.width() * 0.18, top + r.height() * 0.10));
    // 符头
    p.setBrush(c);
    p.drawEllipse(QPointF(x - r.width() * 0.07, bottom), r.width() * 0.12, r.width() * 0.09);
}

// 文档：用于文本/办公类文件。
// 画法：先画白纸外形，再在纸面内部等距画 3 条横线，模拟文字行。
static void drawDoc(QPainter &p, const QRectF &r, const QColor &c)
{
    drawFileBase(p, r, c, c, false);
    QPen pen(c, 1.4);
    p.setPen(pen);
    const QPainterPath path = paperPath(r, 0.30);
    const QRectF br = path.boundingRect();
    for (int i = 1; i <= 3; ++i) {
        const qreal y = br.top() + br.height() * (0.42 + i * 0.14);
        p.drawLine(QPointF(br.left() + br.width() * 0.16, y),
                   QPointF(br.right() - br.width() * 0.20, y));
    }
}

// 图片：用于图像文件。
// 画法：圆角相框 + 左上角小太阳 + 底部起伏的山峦折线，是通用的「图片」符号。
static void drawImage(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(QColor(c.red(), c.green(), c.blue(), 40));
    const qreal w = r.width() * 0.66;
    const qreal h = r.height() * 0.60;
    QRectF box(r.center().x() - w / 2.0, r.center().y() - h / 2.0, w, h);
    p.drawRoundedRect(box, 3.0, 3.0);

    // 小太阳
    p.setBrush(c);
    p.drawEllipse(QPointF(box.left() + w * 0.26, box.top() + h * 0.26), w * 0.07, w * 0.07);

    // 山
    p.setBrush(Qt::NoBrush);
    QPainterPath hill;
    hill.moveTo(box.left() + w * 0.10, box.bottom() - h * 0.12);
    hill.lineTo(box.left() + w * 0.42, box.bottom() - h * 0.52);
    hill.lineTo(box.left() + w * 0.62, box.bottom() - h * 0.24);
    hill.lineTo(box.left() + w * 0.76, box.bottom() - h * 0.40);
    hill.lineTo(box.right() - w * 0.08, box.bottom() - h * 0.12);
    p.drawPath(hill);
}

// 地球：用于网页链接（网址）。
// 画法：一个圆（外轮廓）+ 内部竖椭圆（经线）+ 水平直径线（赤道），构成地球仪符号。
static void drawGlobe(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QPointF ctr = r.center();
    const qreal rad = r.width() * 0.38;

    // 外圆
    p.drawEllipse(ctr, rad, rad);
    // 竖椭圆（经线）
    p.drawEllipse(ctr, rad * 0.45, rad);
    // 横线（赤道）
    p.drawLine(QPointF(ctr.x() - rad, ctr.y()), QPointF(ctr.x() + rad, ctr.y()));
}

// 电源：用于「开机自启」开关。
// 画法：一个上方留缺口的圆弧（60°~300° 之间绘制）+ 顶部一根竖线，
//       是标准的「电源 / 开关」符号。
static void drawPower(QPainter &p, const QRectF &r, const QColor &c)
{
    QPen pen(c, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QPointF ctr = r.center();
    const qreal rad = r.width() * 0.34;

    // 圆弧（上方留缺口）
    QRectF arcBox(ctr.x() - rad, ctr.y() - rad, rad * 2, rad * 2);
    p.drawArc(arcBox, 60 * 16, 240 * 16);

    // 顶部竖线
    p.drawLine(QPointF(ctr.x(), ctr.y() - rad * 1.35),
               QPointF(ctr.x(), ctr.y() - rad * 0.15));
}

// 统一入口：生成图标。
// 步骤：① 建 px×px 的透明画布；② 开启抗锯齿；
//       ③ 按类型分派到对应 drawXxx（绘制区域 r 为 (0,0,px,px)）；④ 包成 QIcon 返回。
QIcon make(IconType type, const QColor &color, int px)
{
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r(0, 0, px, px);

    switch (type) {
    case Plus:   drawPlus(p, r, color);   break;
    case Gear:   drawGear(p, r, color);   break;
    case Status: drawStatus(p, r, color); break;
    case Play:   drawPlay(p, r, color);   break;
    case Trash:  drawTrash(p, r, color);  break;
    case Folder: drawFolder(p, r, color); break;
    case App:    drawApp(p, r, color);    break;
    case Music:  drawMusic(p, r, color);  break;
    case Doc:    drawDoc(p, r, color);    break;
    case Image:  drawImage(p, r, color);  break;
    case Power:  drawPower(p, r, color);  break;
    case Globe:  drawGlobe(p, r, color);  break;
    case File:
    default:     drawFile(p, r, color);   break;
    }
    p.end();
    return QIcon(pm);
}

// 根据路径（或网址）推断应使用的图标类型。
// 判断优先级：① 网址 → 地球；② 目录 → 文件夹；
//             ③ 按扩展名归类到 程序 / 图片 / 音乐 / 文档；④ 都不匹配 → 通用文件。
IconType typeForPath(const QString &path)
{
    // 网页链接
    if (looksLikeUrl(path))
        return Globe;

    const QFileInfo fi(path);
    if (fi.isDir())
        return Folder;

    const QString suffix = fi.suffix().toLower();

    // 常见扩展名分类表（均小写比较）
    static const QStringList imgExt = {"png","jpg","jpeg","bmp","gif","webp","ico","tif","tiff","svg"};
    static const QStringList musExt = {"mp3","wav","flac","aac","ogg","m4a","wma","ape","mid"};
    static const QStringList appExt = {"exe","bat","cmd","com","msi","lnk","app"};
    static const QStringList docExt = {"txt","md","doc","docx","pdf","rtf","odt","xls","xlsx","ppt","pptx","csv","log","json","xml","html","htm","ini","cfg"};

    if (appExt.contains(suffix)) return App;
    if (imgExt.contains(suffix)) return Image;
    if (musExt.contains(suffix)) return Music;
    if (docExt.contains(suffix)) return Doc;
    return File;   // 其他一律用通用文件图标
}

} // namespace IconPainter

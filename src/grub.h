#pragma once
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

// GRUB 引导项（含子菜单嵌套路径，如 "Advanced options for Arch Linux>Arch Linux ..."）
struct GrubEntry {
    QString title;      // 条目标题
    QString path;       // 完整路径（用 > 连接，可直接写入 GRUB_DEFAULT）
    int depth = 0;      // 0 = 顶层
    bool isSubmenu = false;
};

namespace Grub {

QString cfgPath();       // /boot/grub2/grub.cfg | /boot/grub/grub.cfg，找不到为空
QString defaultsPath();  // /etc/default/grub
QString mkconfigCommand(); // "update-grub" | "grub2-mkconfig -o ..." | "grub-mkconfig -o ..."

QVector<GrubEntry> parseMenu(const QString& cfgPath);

// 解析 /etc/default/grub 为 key -> 已去引号的值
QHash<QString, QString> readDefaults();

// 在原文上替换/追加 key=rawValue（rawValue 需自带引号），保留注释与其他行
QString updatedDefaultsText(const QString& oldText, const QHash<QString, QString>& rawPairs);

// 生成 shell 引号包裹的值："abc" （转义内部引号）
QString quoteValue(const QString& v);

} // namespace Grub

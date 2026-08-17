#pragma once
#include <QString>
#include <QStringList>
#include <QVector>

// XDG 标准自启动项（GNOME/KDE/XFCE/lxqt/i3 等全部遵守）
struct AutoInfo {
    QString fileName;     // xxx.desktop
    QString title;        // 本地化 Name
    QString exec;         // Exec=
    QString comment;      // Comment（zh_CN 优先）
    QString userPath;     // ~/.config/autostart/xxx.desktop（存在才非空）
    QString systemPath;   // /etc/xdg/autostart/xxx.desktop（存在才非空）
    bool enabled = true;
    bool managedShadow = false; // 用户目录文件是我们生成的“禁用遮挡”文件
};

namespace Autostart {

QString userDir();    // ~/.config/autostart
QString systemDir();  // /etc/xdg/autostart

QVector<AutoInfo> list();

// 纯用户目录文件操作，无需提权：
//   禁用系统级条目 = 在用户目录写同名 Hidden=true 遮挡文件（XDG 规范）
bool setEnabled(const AutoInfo& info, bool enable, QString* err = nullptr);

} // namespace Autostart

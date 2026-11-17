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
    QString userPath;     // $XDG_CONFIG_HOME/autostart 中的用户文件
    QString systemPath;   // $XDG_CONFIG_DIRS 中优先级最高的同名系统文件
    bool enabled = true;
    bool applicable = true; // 当前桌面匹配且 TryExec 可用；不适用项不能启用
    bool managedShadow = false; // 用户目录文件是我们生成的“禁用遮挡”文件
};

namespace Autostart {

QString userDir();    // XDG_CONFIG_HOME（默认 ~/.config）下的 autostart
QStringList systemDirs(); // XDG_CONFIG_DIRS（默认 /etc/xdg）下的目录，优先级从高到低
QString systemDir();  // 首个系统目录；无有效绝对路径时为空

QVector<AutoInfo> list();

// 纯用户目录文件操作，无需提权：
//   禁用系统级条目 = 在用户目录写同名 Hidden=true 遮挡文件（XDG 规范）
bool setEnabled(const AutoInfo& info, bool enable, QString* err = nullptr);

} // namespace Autostart

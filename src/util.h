#pragma once
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace Util {

// 以 root 身份同步执行 shell 命令的结果
struct RunResult {
    bool ok = false;             // exitCode == 0
    bool asyncLaunched = false;  // 兼容旧调用方；同步执行模式始终为 false
    int exitCode = -1;
    QString output;              // stdout+stderr
    QString error;               // 说明性错误（未启动/超时等）
};

bool amRoot();
bool havePkexec();

// 提权执行：
//   root 直接执行 -> pkexec sh -c（弹 polkit 认证）；缺失时返回错误
RunResult runRoot(const QString& shellCmd, int timeoutMs = 600000);

// ---------- 终端探测（任意 DE/WM） ----------
QString findTerminal();  // 返回终端程序名，找不到返回空串
QStringList terminalWrapArgs(const QString& termProgram, const QString& script);
bool launchTerminalScript(const QString& script);

// ---------- 杂项 ----------
QString shellQuote(const QString& s);
QString distroName();    // "CachyOS" / "Debian GNU/Linux 12" ...
QString initSystem();    // "systemd" / "SysVinit" / ...
QString sessionInfo();   // "Wayland" / "X11" / "TTY" / ...
QString appConfigDir();  // $XDG_CONFIG_HOME/msconfig-linux（默认 ~/.config）

bool loadJson(QJsonObject* obj, QString* err = nullptr); // appConfigDir()/state.json
bool saveJson(const QJsonObject& obj, QString* err);

} // namespace Util

#include "util.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTextStream>
#include <unistd.h>

namespace Util {

bool amRoot()
{
    return ::geteuid() == 0;
}

bool havePkexec()
{
    return !QStandardPaths::findExecutable("pkexec").isEmpty();
}

QString shellQuote(const QString& s)
{
    QString t = s;
    t.replace('\'', "'\\''");
    return "'" + t + "'";
}

static RunResult runSync(const QString& program, const QStringList& args, int timeoutMs)
{
    RunResult r;
    QProcess p;
    p.start(program, args);
    if (!p.waitForStarted(5000)) {
        r.error = QStringLiteral("failed to start %1").arg(program);
        return r;
    }
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        p.waitForFinished(3000);
        r.error = QStringLiteral("%1 timed out").arg(program);
        return r;
    }
    r.exitCode = p.exitCode();
    r.output = QString::fromUtf8(p.readAllStandardOutput());
    r.output += QString::fromUtf8(p.readAllStandardError());
    r.ok = (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0);
    return r;
}

RunResult runRoot(const QString& shellCmd, int timeoutMs)
{
    RunResult r;
    if (amRoot()) {
        return runSync("sh", {"-c", shellCmd}, timeoutMs);
    }
    if (havePkexec()) {
        return runSync("pkexec", {"sh", "-c", shellCmd}, timeoutMs);
    }
    const QString term = findTerminal();
    if (!term.isEmpty()) {
        // 回退：在终端里交互式 sudo（无 pkexec 的极简环境）
        const QString script = "sudo sh -c " + shellQuote(shellCmd)
            + "; printf '\\n'; printf '%s' " + shellQuote("按回车键关闭... / Press Enter to close...")
            + "; read _";
        if (launchTerminalScript(script)) {
            r.asyncLaunched = true;
            return r;
        }
    }
    r.error = "no privilege elevation method available";
    return r;
}

// 各终端的“执行命令”参数风格；未列出的终端一律用 ["-e"]
static const struct {
    const char* program;
    const char* pre[3]; // 以 nullptr 结尾
} kTerminals[] = {
    {"konsole",        {"-e", nullptr}},
    {"gnome-terminal", {"--", nullptr}},
    {"kgx",            {"-e", nullptr}},
    {"kitty",          {nullptr}},
    {"foot",           {nullptr}},
    {"alacritty",      {"-e", nullptr}},
    {"wezterm",        {"start", "--", nullptr}},
    {"xfce4-terminal", {"-x", nullptr}},
    {"qterminal",      {"-e", nullptr}},
    {"lxterminal",     {"-e", nullptr}},
    {"terminator",     {"-x", nullptr}},
    {"tilix",          {"-e", nullptr}},
    {"sakura",         {"-e", nullptr}},
    {"st",             {"-e", nullptr}},
    {"xterm",          {"-e", nullptr}},
    {"termite",        {"-e", nullptr}},
    {"deepin-terminal",{"-e", nullptr}},
};

QStringList terminalWrapArgs(const QString& termProgram, const QString& script)
{
    for (const auto& t : kTerminals) {
        if (termProgram == QLatin1String(t.program)) {
            QStringList args;
            for (int i = 0; t.pre[i]; ++i)
                args << QString::fromLatin1(t.pre[i]);
            args << "sh" << "-c" << script;
            return args;
        }
    }
    return {termProgram == "gnome-terminal" ? "--" : "-e", "sh", "-c", script};
}

QString findTerminal()
{
    const auto env = QProcessEnvironment::systemEnvironment();
    const QString envTerm = env.value("TERMINAL");
    if (!envTerm.isEmpty() && !QStandardPaths::findExecutable(envTerm).isEmpty())
        return envTerm;
    for (const auto& t : kTerminals) {
        const QString name = QString::fromLatin1(t.program);
        if (!QStandardPaths::findExecutable(name).isEmpty())
            return name;
    }
    return QString();
}

bool launchTerminalScript(const QString& script)
{
    const QString term = findTerminal();
    if (term.isEmpty())
        return false;
    return QProcess::startDetached(term, terminalWrapArgs(term, script));
}

QString distroName()
{
    QFile f("/etc/os-release");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString pretty, name;
        while (!f.atEnd()) {
            const QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith("PRETTY_NAME="))
                pretty = line.mid(12).remove('"');
            else if (line.startsWith("NAME="))
                name = line.mid(5).remove('"');
        }
        if (!pretty.isEmpty())
            return pretty;
        if (!name.isEmpty())
            return name;
    }
    return QSysInfo::productType();
}

QString initSystem()
{
    if (!QStandardPaths::findExecutable("systemctl").isEmpty()) {
        QProcess p;
        p.start("systemctl", {"is-system-running"});
        if (p.waitForStarted(2000) && p.waitForFinished(2000)) {
            const QByteArray out = p.readAllStandardOutput().trimmed();
            if (out == "running" || out == "degraded" || out == "starting"
                || out == "maintenance" || out == "stopping")
                return "systemd";
        }
    }
    QFile f("/proc/1/comm");
    if (f.open(QIODevice::ReadOnly)) {
        const QString comm = QString::fromUtf8(f.readAll()).trimmed();
        if (comm == "systemd")
            return "systemd";
        if (comm == "init")
            return "SysVinit";
        return comm;
    }
    return "?";
}

QString sessionInfo()
{
    const auto env = QProcessEnvironment::systemEnvironment();
    if (!env.value("WAYLAND_DISPLAY").isEmpty())
        return "Wayland";
    if (!env.value("DISPLAY").isEmpty())
        return "X11";
    return "TTY";
}

QString appConfigDir()
{
    return QDir::home().filePath(".config/msconfig-linux");
}

bool loadJson(QJsonObject* obj, QString* err)
{
    *obj = {};
    QFile f(QDir(appConfigDir()).filePath("state.json"));
    if (!f.exists())
        return true; // 无状态文件视为空状态，不是错误
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = f.errorString();
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
        if (err) *err = "state.json is not a JSON object";
        return false;
    }
    *obj = doc.object();
    return true;
}

bool saveJson(const QJsonObject& obj, QString* err)
{
    QDir d;
    if (!d.mkpath(appConfigDir())) {
        if (err) *err = "cannot create " + appConfigDir();
        return false;
    }
    QFile f(QDir(appConfigDir()).filePath("state.json"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (err) *err = f.errorString();
        return false;
    }
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace Util

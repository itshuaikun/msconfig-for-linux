#include "autostart.h"

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QTextStream>
#include <QSaveFile>
#include <QStandardPaths>
#include <QFileInfo>
#include <QMap>

namespace Autostart {

// 保序 .desktop 迷你解析器：只改目标键，保留其余行
class DesktopFile {
public:
    bool load(const QString& path)
    {
        m_lines.clear();
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QTextStream in(&f);
        while (!in.atEnd())
            m_lines.append(in.readLine());
        return true;
    }

    bool save(const QString& path) const
    {
        QSaveFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;
        QTextStream out(&f);
        for (const QString& l : m_lines)
            out << l << '\n';
        out.flush();
        if (out.status() != QTextStream::Ok || f.error() != QFileDevice::NoError) {
            f.cancelWriting();
            return false;
        }
        return f.commit();
    }

    // group 下 key= 的值（取该组内最后一次出现）
    QString value(const QString& group, const QString& key, const QString& def = QString()) const
    {
        int gi = findGroup(group);
        if (gi < 0)
            return def;
        for (int i = groupEnd(gi) - 1; i > gi; --i) {
            const QString v = matchKey(m_lines[i], key);
            if (!v.isNull())
                return v;
        }
        return def;
    }

    void setValue(const QString& group, const QString& key, const QString& val)
    {
        int gi = findGroup(group);
        if (gi < 0) {
            m_lines.append("[" + group + "]");
            gi = m_lines.size() - 1;
        }
        for (int i = groupEnd(gi) - 1; i > gi; --i) {
            if (!matchKey(m_lines[i], key).isNull()) {
                m_lines[i] = key + "=" + val;
                return;
            }
        }
        m_lines.insert(gi + 1, key + "=" + val);
    }

    void removeKey(const QString& group, const QString& key)
    {
        int gi = findGroup(group);
        if (gi < 0)
            return;
        for (int i = groupEnd(gi) - 1; i > gi; --i) {
            if (!matchKey(m_lines[i], key).isNull())
                m_lines.removeAt(i);
        }
    }

private:
    int groupEnd(int start) const
    {
        for (int i = start + 1; i < m_lines.size(); ++i) {
            if (isGroupLine(m_lines[i]))
                return i;
        }
        return m_lines.size();
    }

    static bool isGroupLine(const QString& l)
    {
        return l.trimmed().startsWith('[');
    }

    static QString matchKey(const QString& line, const QString& key)
    {
        if (line.startsWith(key + "="))
            return line.mid(key.size() + 1);
        return QString(); // null = 不匹配
    }

    int findGroup(const QString& group) const
    {
        const QString tag = "[" + group + "]";
        for (int i = 0; i < m_lines.size(); ++i) {
            if (m_lines[i].simplified() == tag)
                return i;
        }
        return -1;
    }

    QStringList m_lines;
};

static QString langSuffix()
{
    const QLocale loc = QLocale::system();
    QString s = loc.name(); // 如 "zh_CN"
    if (s.isEmpty() || s == "C")
        return QString();
    return s.replace('-', '_');
}

// 取 .desktop 的 Name/Comment，优先本地化键（Name[zh_CN]）
static QString localizedValue(DesktopFile& df, const QString& key, const QString& fallback)
{
    const QString suffix = langSuffix();
    if (!suffix.isEmpty()) {
        const QString v = df.value("Desktop Entry", key + "[" + suffix + "]");
        if (!v.isEmpty())
            return v;
        const QString lang = suffix.section('_', 0, 0);
        const QString v2 = df.value("Desktop Entry", key + "[" + lang + "]");
        if (!v2.isEmpty())
            return v2;
    }
    const QString v3 = df.value("Desktop Entry", key);
    return v3.isEmpty() ? fallback : v3;
}

QString userDir()
{
    const QString configHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    return QDir(QDir::isAbsolutePath(configHome) ? configHome
                                                : QDir::home().filePath(".config"))
        .filePath("autostart");
}

QStringList systemDirs()
{
    const QString configDirs = qEnvironmentVariable("XDG_CONFIG_DIRS");
    QStringList result;
    const QStringList roots = (configDirs.isEmpty() ? QString("/etc/xdg") : configDirs)
                                 .split(':', Qt::SkipEmptyParts);
    for (const QString& root : roots) {
        if (!QDir::isAbsolutePath(root))
            continue;
        const QString dir = QDir::cleanPath(QDir(root).filePath("autostart"));
        if (!result.contains(dir))
            result.append(dir);
    }
    return result;
}

QString systemDir()
{
    return systemDirs().value(0);
}

static QVector<AutoInfo> scanDir(const QString& dir, bool user)
{
    QVector<AutoInfo> out;
    QDir d(dir);
    const QStringList files = d.entryList({"*.desktop"}, QDir::Files);
    for (const QString& name : files) {
        AutoInfo a;
        a.fileName = name;
        a.userPath = user ? d.filePath(name) : QString();
        a.systemPath = user ? QString() : d.filePath(name);
        out.append(a);
    }
    return out;
}

// Desktop filters constrain applicability independently of the user's enabled flag.
static bool applicable(const DesktopFile& df)
{
    const QString type = df.value("Desktop Entry", "Type");
    if (type != "Application" || df.value("Desktop Entry", "Exec").isEmpty())
        return false;
    QStringList desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP")
                               .split(':', Qt::SkipEmptyParts);
    // Older KDE sessions may only export this compatibility variable.
    if (desktops.isEmpty() && qEnvironmentVariable("KDE_FULL_SESSION") == "true")
        desktops.append("KDE");
    const QString only = df.value("Desktop Entry", "OnlyShowIn");
    const QString excluded = df.value("Desktop Entry", "NotShowIn");
    const QStringList allowed = only.split(';', Qt::SkipEmptyParts);
    const QStringList denied = excluded.split(';', Qt::SkipEmptyParts);
    bool matches = only.isNull();
    for (const QString& desktop : desktops) {
        if (denied.contains(desktop))
            return false;
        if (allowed.contains(desktop))
            matches = true;
    }
    if (!matches)
        return false;
    const QString tryExec = df.value("Desktop Entry", "TryExec");
    if (!tryExec.isEmpty()) {
        if (QDir::isAbsolutePath(tryExec)) {
            const QFileInfo executable(tryExec);
            if (!executable.isFile() || !executable.isExecutable())
                return false;
        } else if (tryExec.contains('/') || QStandardPaths::findExecutable(tryExec).isEmpty()) {
            return false;
        }
    }
    return true;
}

QVector<AutoInfo> list()
{
    // 名称 -> 条目（用户目录条目遮挡同名的系统条目）
    QMap<QString, AutoInfo> merged;
    for (const QString& dir : systemDirs()) {
        for (const AutoInfo& a : scanDir(dir, false)) {
            if (!merged.contains(a.fileName))
                merged.insert(a.fileName, a);
        }
    }
    for (const AutoInfo& a : scanDir(userDir(), true)) {
        if (merged.contains(a.fileName)) {
            merged[a.fileName].userPath = a.userPath;
        } else {
            merged[a.fileName] = a;
        }
    }

    QVector<AutoInfo> out;
    for (auto it = merged.begin(); it != merged.end(); ++it) {
        AutoInfo& a = it.value();
        DesktopFile userDf;
        const bool hasUser = !a.userPath.isEmpty() && userDf.load(a.userPath);
        DesktopFile sysDf;
        const bool hasSys = !a.systemPath.isEmpty() && sysDf.load(a.systemPath);

        // 生效文件 = 用户条目（若存在），否则系统条目
        DesktopFile* eff = !a.userPath.isEmpty() ? (hasUser ? &userDf : nullptr)
                                                : (hasSys ? &sysDf : nullptr);
        if (!eff)
            continue;

        if (hasUser) {
            a.title = localizedValue(userDf, "Name", a.fileName);
            a.exec = userDf.value("Desktop Entry", "Exec");
            a.comment = localizedValue(userDf, "Comment", QString());
            // 只有标记行的“空壳”文件 = 我们生成的遮挡
            a.managedShadow = userDf.value("Desktop Entry", "X-msconfig-Managed") == "true";
            if (a.title == a.fileName)
                a.title.clear();
        }
        if (hasSys && (!hasUser || a.managedShadow)) {
            const QString sysTitle = localizedValue(sysDf, "Name", QString());
            if (a.title.isEmpty() && !sysTitle.isEmpty())
                a.title = sysTitle;
            if (a.exec.isEmpty())
                a.exec = sysDf.value("Desktop Entry", "Exec");
            if (a.comment.isEmpty())
                a.comment = localizedValue(sysDf, "Comment", QString());
        }
        if (a.title.isEmpty())
            a.title = a.fileName;

        const QString hidden = eff->value("Desktop Entry", "Hidden");
        const QString gnomeEnabled = eff->value("Desktop Entry", "X-GNOME-Autostart-enabled");
        a.applicable = applicable(a.managedShadow && hasSys ? sysDf : *eff);
        a.enabled = a.applicable && !(hidden.compare("true", Qt::CaseInsensitive) == 0)
                    && gnomeEnabled.compare("false", Qt::CaseInsensitive) != 0;
        out.append(a);
    }
    return out;
}

bool setEnabled(const AutoInfo& info, bool enable, QString* err)
{
    if (err)
        err->clear();
    if (info.fileName.isEmpty() || info.fileName == "." || info.fileName == ".."
        || info.fileName.contains('/') || info.fileName.contains('\\')) {
        if (err) *err = "invalid autostart filename";
        return false;
    }
    const QString udir = userDir();
    if (!QDir().mkpath(udir)) {
        if (err) *err = "cannot create " + udir;
        return false;
    }
    const QString ufile = QDir(udir).filePath(info.fileName);

    if (enable) {
        DesktopFile df;
        const bool userExists = QFileInfo::exists(ufile);
        if (userExists && !df.load(ufile)) {
            if (err) *err = "cannot read " + ufile;
            return false;
        }
        const bool shadow = userExists
            && df.value("Desktop Entry", "X-msconfig-Managed") == "true";
        if (!userExists || shadow) {
            if (info.systemPath.isEmpty() || !df.load(info.systemPath)) {
                if (err) *err = "cannot read " + info.systemPath;
                return false;
            }
        }
        if (!applicable(df)) {
            if (err) *err = "autostart entry is not applicable to this desktop or TryExec is unavailable";
            return false;
        }
        // Removing a managed shadow only enables a system entry that is itself enabled.
        // Otherwise copy the full system entry and clear its disabled flags locally.
        if (shadow && df.value("Desktop Entry", "Hidden").compare("true", Qt::CaseInsensitive) != 0
            && df.value("Desktop Entry", "X-GNOME-Autostart-enabled").compare("false", Qt::CaseInsensitive) != 0) {
            if (!QFile::remove(ufile)) {
                if (err) *err = "cannot remove " + ufile;
                return false;
            }
            return true;
        }
        df.removeKey("Desktop Entry", "Hidden");
        df.removeKey("Desktop Entry", "X-msconfig-Managed");
        df.setValue("Desktop Entry", "X-GNOME-Autostart-enabled", "true");
        if (!df.save(ufile)) {
            if (err) *err = "cannot write " + ufile;
            return false;
        }
        return true;
    }

    if (info.userPath.isEmpty()) {
        // 禁用系统级条目：写用户级遮挡文件（XDG 覆盖规则）
        DesktopFile df;
        df.setValue("Desktop Entry", "Type", "Application");
        df.setValue("Desktop Entry", "Hidden", "true");
        df.setValue("Desktop Entry", "X-msconfig-Managed", "true");
        if (!df.save(ufile)) {
            if (err) *err = "cannot write " + ufile;
            return false;
        }
        return true;
    }

    DesktopFile df;
    if (!df.load(info.userPath)) {
        if (err) *err = "cannot read " + info.userPath;
        return false;
    }
    df.setValue("Desktop Entry", "Hidden", "true");
    df.setValue("Desktop Entry", "X-GNOME-Autostart-enabled", "false");
    if (!df.save(info.userPath)) {
        if (err) *err = "cannot write " + info.userPath;
        return false;
    }
    return true;
}

} // namespace Autostart

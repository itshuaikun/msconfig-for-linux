#include "autostart.h"

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QTextStream>

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
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return false;
        QTextStream out(&f);
        for (const QString& l : m_lines)
            out << l << '\n';
        return true;
    }

    // group 下 key= 的值（取该组内最后一次出现）
    QString value(const QString& group, const QString& key, const QString& def = QString()) const
    {
        int gi = findGroup(group);
        if (gi < 0)
            return def;
        for (int i = m_lines.size() - 1; i > gi; --i) {
            if (isGroupLine(m_lines[i]))
                break;
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
        for (int i = m_lines.size() - 1; i > gi; --i) {
            if (isGroupLine(m_lines[i]))
                break;
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
        for (int i = m_lines.size() - 1; i > gi; --i) {
            if (isGroupLine(m_lines[i]))
                break;
            if (!matchKey(m_lines[i], key).isNull()) {
                m_lines.removeAt(i);
                return;
            }
        }
    }

private:
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
    return QDir::home().filePath(".config/autostart");
}

QString systemDir()
{
    return "/etc/xdg/autostart";
}

static QVector<AutoInfo> scanDir(const QString& dir)
{
    QVector<AutoInfo> out;
    QDir d(dir);
    const QStringList files = d.entryList({"*.desktop"}, QDir::Files);
    for (const QString& name : files) {
        AutoInfo a;
        a.fileName = name;
        a.userPath = (dir == userDir()) ? d.filePath(name) : QString();
        a.systemPath = (dir == systemDir()) ? d.filePath(name) : QString();
        out.append(a);
    }
    return out;
}

QVector<AutoInfo> list()
{
    // 名称 -> 条目（用户目录条目遮挡同名的系统条目）
    QMap<QString, AutoInfo> merged;
    for (const AutoInfo& a : scanDir(systemDir()))
        merged[a.fileName] = a;
    for (const AutoInfo& a : scanDir(userDir())) {
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
        DesktopFile* eff = hasUser ? &userDf : (hasSys ? &sysDf : nullptr);
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
        if (hasSys) {
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
        a.enabled = !(hidden.compare("true", Qt::CaseInsensitive) == 0)
                    && gnomeEnabled.compare("false", Qt::CaseInsensitive) != 0;
        out.append(a);
    }
    return out;
}

bool setEnabled(const AutoInfo& info, bool enable, QString* err)
{
    const QString udir = userDir();
    if (!QDir().mkpath(udir)) {
        if (err) *err = "cannot create " + udir;
        return false;
    }
    const QString ufile = QDir(udir).filePath(info.fileName);

    if (enable) {
        if (info.managedShadow) {
            // 删除遮挡文件，露出系统条目
            if (!QFile::remove(ufile)) {
                if (err) *err = "cannot remove " + ufile;
                return false;
            }
            return true;
        }
        DesktopFile df;
        if (!df.load(ufile)) {
            if (err) *err = "cannot read " + ufile;
            return false;
        }
        df.removeKey("Desktop Entry", "Hidden");
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

#include "grub.h"

#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include "util.h"

namespace Grub {

QString cfgPath()
{
    const QStringList candidates = {
        "/boot/grub2/grub.cfg",   // Fedora/RHEL/openSUSE
        "/boot/grub/grub.cfg",    // Arch/Debian/Ubuntu 等
    };
    for (const QString& p : candidates) {
        if (QFile::exists(p))
            return p;
    }
    return QString();
}

QString defaultsPath()
{
    return "/etc/default/grub";
}

QString mkconfigCommand()
{
    const QString cfg = cfgPath();
    if (!QStandardPaths::findExecutable("update-grub").isEmpty())
        return "update-grub"; // Debian/Ubuntu 及衍生
    const QString mk = !QStandardPaths::findExecutable("grub2-mkconfig").isEmpty()
        ? "grub2-mkconfig"
        : "grub-mkconfig";
    return mk + " -o " + Util::shellQuote(cfg);
}

QVector<GrubEntry> parseMenu(const QString& cfgPath)
{
    QVector<GrubEntry> entries;
    QFile f(cfgPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return entries;

    // menuentry/submenu 的标题（单双引号皆可）
    static const QRegularExpression re(
        R"(^\s*(menuentry|submenu)\s+(['"])(.*?)\2)");

    QStringList stack; // 已打开的 submenu 标题
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const QRegularExpressionMatch m = re.match(line);
        if (m.hasMatch()) {
            GrubEntry e;
            e.isSubmenu = (m.captured(1) == "submenu");
            e.title = m.captured(3);
            e.depth = stack.size();
            e.path = stack.isEmpty() ? e.title : stack.join('>') + '>' + e.title;
            entries.append(e);
            // "menuentry 'X' {...}"（如 {reboot}）在同一行开合，净括号为 0，不入栈
            if (line.count('{') > line.count('}'))
                stack.append(e.title);
        } else if (line.trimmed() == "}") {
            if (!stack.isEmpty())
                stack.removeLast();
        }
    }
    return entries;
}

static QString stripQuotes(const QString& v)
{
    const QString t = v.trimmed();
    if (t.size() >= 2) {
        if (t.startsWith('\'') && t.endsWith('\''))
            return t.mid(1, t.size() - 2);
        if (t.startsWith('"') && t.endsWith('"')) {
            QString out;
            const QString inner = t.mid(1, t.size() - 2);
            for (int i = 0; i < inner.size(); ++i) {
                if (inner[i] == '\\' && i + 1 < inner.size()
                    && QString("\\\"$`").contains(inner[i + 1]))
                    ++i;
                out += inner[i];
            }
            return out;
        }
    }
    return t;
}

QHash<QString, QString> readDefaults()
{
    QHash<QString, QString> map;
    QFile f(defaultsPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return map;
    static const QRegularExpression re(R"(^\s*(GRUB_[A-Z0-9_]+)\s*=\s*(.*)$)");
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QRegularExpressionMatch m = re.match(in.readLine());
        if (m.hasMatch())
            map.insert(m.captured(1), stripQuotes(m.captured(2)));
    }
    return map;
}

QString updatedDefaultsText(const QString& oldText, const QHash<QString, QString>& rawPairs)
{
    QStringList lines = oldText.split('\n');
    QHash<QString, bool> done;
    for (int i = 0; i < lines.size(); ++i) {
        static const QRegularExpression re(R"(^\s*(GRUB_[A-Z0-9_]+)\s*=)");
        const QRegularExpressionMatch m = re.match(lines[i]);
        if (!m.hasMatch())
            continue;
        const QString key = m.captured(1);
        if (!rawPairs.contains(key))
            continue;
        lines[i] = key + "=" + rawPairs.value(key);
        done[key] = true;
    }
    // 未出现的 key 追加到末尾
    QStringList append;
    for (auto it = rawPairs.constBegin(); it != rawPairs.constEnd(); ++it) {
        if (!done.value(it.key()))
            append << it.key() + "=" + it.value();
    }
    QString out = lines.join('\n');
    if (!append.isEmpty()) {
        if (!out.endsWith('\n') && !out.isEmpty())
            out += '\n';
        out += append.join('\n') + '\n';
    }
    return out;
}

QString quoteValue(const QString& v)
{
    QString t = v;
    t.replace('\\', "\\\\").replace('"', "\\\"");
    t.replace('$', "\\$").replace('`', "\\`");
    return '"' + t + '"';
}

} // namespace Grub

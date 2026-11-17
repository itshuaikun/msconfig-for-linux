#include "services.h"

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>

namespace Services {

static QStringList unitDirs()
{
    // systemd 单元查找顺序
    return {
        "/etc/systemd/system",
        "/run/systemd/system",
        "/usr/local/lib/systemd/system",
        "/usr/lib/systemd/system",
        "/lib/systemd/system",
    };
}

QVector<ServiceInfo> list(QString* err)
{
    QVector<ServiceInfo> out;
    QProcess p;
    p.start("systemctl", {"list-unit-files", "--type=service", "--no-legend", "--no-pager"});
    if (!p.waitForStarted(5000) || !p.waitForFinished(30000)) {
        if (err)
            *err = QString::fromUtf8(p.readAllStandardError());
        return out;
    }
    if (p.exitCode() != 0) {
        if (err)
            *err = QString::fromUtf8(p.readAllStandardError());
        return out;
    }
    const QStringList lines = QString::fromUtf8(p.readAllStandardOutput()).split('\n');
    const QStringList dirs = unitDirs();

    for (const QString& line : lines) {
        const QStringList cols = line.simplified().split(' ');
        if (cols.size() < 2 || !cols.at(0).endsWith(".service"))
            continue;
        ServiceInfo s;
        s.unit = cols.at(0);
        s.state = cols.at(1);
        s.toggleable = (s.state == "enabled" || s.state == "disabled");

        for (const QString& d : dirs) {
            const QString path = d + '/' + s.unit;
            if (QFile::exists(path)) {
                s.path = path;
                s.userInstalled = (d.startsWith("/etc/") || d.startsWith("/run/")
                                   || d.startsWith("/usr/local/"));
                s.modified = QFileInfo(path).lastModified();
                // Description=
                QFile uf(path);
                if (uf.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    static const QRegularExpression dre(
                        R"(^\s*Description\s*=\s*(.*)$)",
                        QRegularExpression::CaseInsensitiveOption);
                    QTextStream tin(&uf);
                    while (!tin.atEnd()) {
                        const QRegularExpressionMatch m = dre.match(tin.readLine());
                        if (m.hasMatch()) {
                            s.desc = m.captured(1).trimmed();
                            break;
                        }
                    }
                }
                break;
            }
        }
        if (s.path.isEmpty())
            s.userInstalled = false; // 找不到文件（如 indirect/generated）不参与过滤
        out.append(s);
    }
    return out;
}

bool isProtected(const QString& unit)
{
    static const QStringList prefixes = {
        "systemd-", "dbus", "polkit", "getty@", "user@",
        "NetworkManager", "wpa_supplicant", "dhcpcd", "iwd",
        "display-manager", "sddm", "gdm", "lightdm", "lxdm", "greetd",
    };
    for (const QString& p : prefixes) {
        if (unit.startsWith(p, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

} // namespace Services

// Standalone regression test; link with src/autostart.cpp and Qt Core (no QtTest).
#include "autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <cstdio>
#include <cstdlib>

static void check(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void put(const QString& path, const QByteArray& contents)
{
    check(QDir().mkpath(QFileInfo(path).absolutePath()), "create fixture directory");
    QFile file(path);
    check(file.open(QIODevice::WriteOnly), "open fixture");
    check(file.write(contents) == contents.size(), "write fixture");
    file.close();
}

static QByteArray get(const QString& path)
{
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "read fixture");
    return file.readAll();
}

static AutoInfo entry(const QString& name)
{
    for (const AutoInfo& info : Autostart::list()) {
        if (info.fileName == name)
            return info;
    }
    std::fprintf(stderr, "Missing entry: %s\n", qPrintable(name));
    std::exit(1);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    check(temporary.isValid(), "temporary directory");
    const QString base = temporary.path();
    const QString user = base + "/user";
    const QString first = base + "/first";
    const QString second = base + "/second";
    // Configuration files and executable lookup remain in this temporary tree.
    qputenv("XDG_CONFIG_HOME", user.toUtf8());
    qputenv("XDG_CONFIG_DIRS", (first + ":relative::" + second + ":" + first).toUtf8());
    qputenv("XDG_CURRENT_DESKTOP", "KDE");
    qunsetenv("KDE_FULL_SESSION");
    qputenv("PATH", (base + "/bin").toUtf8());
    const QString u = user + "/autostart/";
    const QString s1 = first + "/autostart/";
    const QString s2 = second + "/autostart/";
    check(Autostart::userDir() == user + "/autostart", "XDG_CONFIG_HOME");
    check(Autostart::systemDirs() == QStringList({first + "/autostart", second + "/autostart"}),
          "XDG_CONFIG_DIRS order, invalid roots, duplicates");
    const QByteArray basic = "[Desktop Entry]\nType=Application\nName=Main\nExec=example\n";
    put(s1 + "priority.desktop", basic + "Hidden=true\n");
    put(s2 + "priority.desktop", basic + "Hidden=false\n");
    check(entry("priority.desktop").systemPath == s1 + "priority.desktop", "first system directory wins");
    check(!entry("priority.desktop").enabled, "hidden first entry masks lower directories");
    put(u + "priority.desktop", basic + "Name=User\n");
    check(entry("priority.desktop").title == "User" && entry("priority.desktop").enabled,
          "user entry wins over system");

    const QByteArray actions = "[Desktop Action Other]\nName=Action\nExec=action\nHidden=false\n"
                               "X-GNOME-Autostart-enabled=false\n";
    const QString sectionPath = u + "sections.desktop";
    put(sectionPath, "[Other]\nHidden=true\n" + basic + "Hidden=true\nHidden=true\n" + actions);
    check(!entry("sections.desktop").enabled && entry("sections.desktop").exec == "example",
          "read keys only inside Desktop Entry");
    QString error;
    check(Autostart::setEnabled(entry("sections.desktop"), true, &error), "enable multi-section entry");
    check(entry("sections.desktop").enabled, "remove every duplicate Hidden in target section");
    check(get(sectionPath).endsWith(actions), "enable preserves later section");
    check(get(sectionPath).startsWith("[Other]\nHidden=true\n"), "preserve preceding section");
    check(Autostart::setEnabled(entry("sections.desktop"), false, &error), "disable multi-section entry");
    check(!entry("sections.desktop").enabled, "insert and update keys in target section");
    check(get(sectionPath).endsWith(actions), "disable preserves later section");

    put(s1 + "hidden.desktop", basic + "Hidden=true\n" + actions);
    const QByteArray original = get(s1 + "hidden.desktop");
    check(Autostart::setEnabled(entry("hidden.desktop"), true, &error), "enable system Hidden=true without user file");
    check(entry("hidden.desktop").enabled, "system hidden override enabled");
    check(get(s1 + "hidden.desktop") == original, "system file is never modified");
    check(get(u + "hidden.desktop").contains("Exec=example\n"), "override copies complete system entry");
    check(get(u + "hidden.desktop").endsWith(actions), "system override preserves actions");

    put(s1 + "shadow.desktop", basic);
    check(Autostart::setEnabled(entry("shadow.desktop"), false, &error), "create managed shadow");
    check(entry("shadow.desktop").managedShadow && !entry("shadow.desktop").enabled,
          "managed shadow recognized");
    check(Autostart::setEnabled(entry("shadow.desktop"), true, &error), "remove managed shadow");
    check(!QFile::exists(u + "shadow.desktop") && entry("shadow.desktop").enabled,
          "removing shadow restores enabled system entry");
    check(Autostart::setEnabled(entry("shadow.desktop"), false, &error), "recreate shadow");
    put(s1 + "shadow.desktop", basic + "Hidden=true\n");
    check(Autostart::setEnabled(entry("shadow.desktop"), true, &error), "enable shadow over disabled system entry");
    check(entry("shadow.desktop").enabled && !entry("shadow.desktop").managedShadow,
          "disabled system entry gets complete local override");

    put(u + "kde.desktop", basic + "OnlyShowIn=KDE;\n");
    put(u + "gnome.desktop", basic + "OnlyShowIn=GNOME;\n");
    put(u + "not-kde.desktop", basic + "NotShowIn=KDE;\n");
    put(u + "missing.desktop", basic + "TryExec=missing-test-executable\n");
    check(entry("kde.desktop").enabled, "KDE OnlyShowIn applies");
    for (const QString& name : {QString("gnome.desktop"), QString("not-kde.desktop"), QString("missing.desktop")}) {
        const AutoInfo info = entry(name);
        check(!info.applicable && !info.enabled, "desktop and TryExec exclusions are not enabled");
        const QByteArray before = get(u + name);
        check(!Autostart::setEnabled(info, true, &error) && !error.isEmpty(), "reject enabling inapplicable entry");
        check(get(u + name) == before, "inapplicable entry unchanged");
    }
    qputenv("XDG_CURRENT_DESKTOP", "GNOME:KDE");
    check(entry("gnome.desktop").enabled, "colon-separated desktop identifiers");
    check(!entry("not-kde.desktop").enabled, "NotShowIn vetoes matching desktop");
    qunsetenv("XDG_CURRENT_DESKTOP");
    qputenv("KDE_FULL_SESSION", "true");
    check(entry("kde.desktop").enabled, "legacy KDE session fallback");
    qunsetenv("KDE_FULL_SESSION");
    check(!entry("kde.desktop").enabled, "unknown desktop cannot satisfy OnlyShowIn");

    const QString executable = base + "/bin/available";
    put(executable, "#!/bin/sh\nexit 0\n");
    check(QFile::setPermissions(executable, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner), "executable permissions");
    put(u + "try-name.desktop", basic + "TryExec=available\n");
    put(u + "try-absolute.desktop", basic + "TryExec=" + executable.toUtf8() + "\n");
    check(entry("try-name.desktop").enabled && entry("try-absolute.desktop").enabled, "TryExec via PATH and absolute path");
    check(QFile::setPermissions(executable, QFile::ReadOwner | QFile::WriteOwner), "remove executable permissions");
    check(!entry("try-name.desktop").enabled && !entry("try-absolute.desktop").enabled, "non-executable TryExec rejected");
    put(u + "try-directory.desktop", basic + "TryExec=" + base.toUtf8() + "\n");
    check(!entry("try-directory.desktop").enabled, "TryExec directory rejected");

    put(s1 + "write-error.desktop", basic);
    const AutoInfo writeError = entry("write-error.desktop");
    check(QDir().mkpath(u + "write-error.desktop"), "create conflicting destination directory");
    check(!Autostart::setEnabled(writeError, false, &error) && !error.isEmpty(), "save errors reported");
    check(QFileInfo(u + "write-error.desktop").isDir(), "failed save preserves destination");

    qputenv("XDG_CONFIG_HOME", "relative");
    check(Autostart::userDir() == QDir::home().filePath(".config/autostart"),
          "relative config home ignored (string comparison only)");
    qunsetenv("XDG_CONFIG_DIRS");
    check(Autostart::systemDirs() == QStringList({"/etc/xdg/autostart"}), "default system directories (no filesystem access)");
    std::puts("PASS: autostart regression tests");
    return 0;
}

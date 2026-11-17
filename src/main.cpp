#include <QApplication>
#include <QFileInfo>
#include <QTextStream>

#include "autostart.h"
#include "bootloader.h"
#include "grub.h"
#include "lang.h"
#include "mainwindow.h"
#include "services.h"
#include "util.h"

// 无 GUI 的后端自检：./msconfig --check
static int selfCheck()
{
    QTextStream out(stdout);
    initLang();

    out << "=== msconfig for Linux --check ===\n";
    out << "distro   : " << Util::distroName() << "\n";
    out << "session  : " << Util::sessionInfo() << "\n";
    out << "init     : " << Util::initSystem() << "\n";
    out << "root     : " << (Util::amRoot() ? "yes" : "no") << "\n";
    out << "pkexec   : " << (Util::havePkexec() ? "found" : "not found") << "\n";
    out << "terminal : "
        << (Util::findTerminal().isEmpty() ? "(none)" : Util::findTerminal()) << "\n";

    const auto loader = Bootloader::detect();
    out << "bootloader: " << loader.name << "\n";
    out << "boot entry: " << loader.selectedEntry << "\n";
    out << "boot editing: " << (loader.isLimine ? "unavailable (Limine information only)" : "GRUB only") << "\n";

    const QString cfg = Grub::cfgPath();
    out << "grub.cfg : " << (cfg.isEmpty() ? "(not found)" : cfg) << "\n";
    if (!cfg.isEmpty()) {
        out << "mkconfig : " << Grub::mkconfigCommand() << "\n";
        const auto entries = Grub::parseMenu(cfg);
        out << "entries  : " << entries.size() << "\n";
        for (const GrubEntry& e : entries)
            out << "  [" << e.depth << (e.isSubmenu ? " submenu  ] " : " menuentry] ")
                << e.path << "\n";
        const auto defs = Grub::readDefaults();
        for (auto it = defs.constBegin(); it != defs.constEnd(); ++it)
            out << "  default " << it.key() << "=" << it.value() << "\n";
    }

    QString err;
    const auto svcs = Services::list(&err);
    out << "services : " << svcs.size() << (err.isEmpty() ? "" : "  (err: " + err + ")") << "\n";
    int toggleable = 0;
    for (const ServiceInfo& s : svcs)
        if (s.toggleable)
            ++toggleable;
    out << "  toggleable: " << toggleable << "\n";

    const auto autos = Autostart::list();
    out << "autostart: " << autos.size() << " ("
        << Autostart::systemDirs().join(" + ") << " + " << Autostart::userDir() << ")\n";
    for (const AutoInfo& a : autos)
        out << "  " << (a.enabled ? "[on ] " : "[off] ") << a.fileName
            << "  " << a.title << "\n";

    // /etc/default/grub 改写回环测试（不落盘）
    const QString sample = "# comment\nGRUB_DEFAULT=saved\nGRUB_TIMEOUT=5\n";
    const QString updated = Grub::updatedDefaultsText(
        sample, {{"GRUB_TIMEOUT", "10"}, {"GRUB_CMDLINE_LINUX_DEFAULT", "\"loglevel=3\""}});
    const bool roundtripOk = updated.contains("GRUB_TIMEOUT=10")
                             && updated.contains("GRUB_CMDLINE_LINUX_DEFAULT=\"loglevel=3\"")
                             && updated.contains("# comment")
                             && updated.contains("GRUB_DEFAULT=saved");
    out << "grub-default rewrite test: " << (roundtripOk ? "PASS" : "FAIL") << "\n";

    out << "=== check done ===\n";
    return roundtripOk ? 0 : 1;
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--check") {
            QCoreApplication app(argc, argv);
            return selfCheck();
        }
        if (arg == "--version") {
            QTextStream(stdout) << "msconfig for Linux 1.0.0 (Qt " << qVersion() << ")\n";
            return 0;
        }
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName("msconfig-linux");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setWindowIcon(QIcon::fromTheme("utilities-system-monitor"));

    initLang();
    MainWindow w;
    w.show();
    return app.exec();
}

#include "bootloader.h"
#include "grub.h"
#include "util.h"
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    int failures = 0;
    auto check = [&](bool ok, const char* name) {
        QTextStream(stdout) << (ok ? "PASS " : "FAIL ") << name << '\n';
        if (!ok) ++failures;
    };
    QByteArray efi(4, '\0');
    for (const QChar c : QString("Limine 12.10.0")) {
        efi.append(char(c.unicode() & 255));
        efi.append(char(c.unicode() >> 8));
    }
    efi.append(QByteArray(2, '\0'));
    check(Bootloader::decodeEfiString(efi) == "Limine 12.10.0", "EFI UTF-16LE payload");
    check(Bootloader::decodeEfiString(QByteArray(5, '\0')).isEmpty(), "reject truncated EFI payload");

    const QString literal = "quiet $(printf INJECTED) `printf INJECTED` $HOME \\\"'";
    QProcess sh;
    sh.start("/bin/sh", {"-c", "value=" + Grub::quoteValue(literal) + "; printf '%s' \"$value\""});
    check(sh.waitForFinished() && sh.exitCode() == 0 && QString::fromUtf8(sh.readAllStandardOutput()) == literal,
          "GRUB values remain literal when sourced by shell");
    const QString rewritten = Grub::updatedDefaultsText("# preserved\nGRUB_TIMEOUT=2\nGRUB_TIMEOUT=3\n", {{"GRUB_TIMEOUT", "9"}});
    check(rewritten.count("GRUB_TIMEOUT=9") == 2 && rewritten.contains("# preserved"), "duplicate assignments cannot override edits");

    QTemporaryDir temp;
    if (!temp.isValid()) return 1;
    qputenv("XDG_CONFIG_HOME", temp.path().toUtf8());
    QString error;
    QJsonObject obj{{"mode", "normal"}}, loaded;
    check(Util::saveJson(obj, &error) && Util::loadJson(&loaded, &error) && loaded == obj,
          "atomic state persistence in XDG_CONFIG_HOME");
    QFile broken(Util::appConfigDir() + "/state.json");
    check(broken.open(QIODevice::WriteOnly), "open isolated invalid state fixture");
    broken.write("broken"); broken.close();
    check(!Util::loadJson(&loaded, &error) && !error.isEmpty(), "invalid recovery state reports failure");
    return failures ? 1 : 0;
}

#include "mainwindow.h"
#include "generaltab.h"
#include "boottab.h"
#include "bootloader.h"
#include "lang.h"
#include "util.h"
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTextStream>

int main(int argc, char** argv)
{
    QTemporaryDir temp;
    if (!temp.isValid()) return 1;
    qputenv("XDG_CONFIG_HOME", temp.path().toUtf8());
    QApplication app(argc, argv);
    initLang();
    MainWindow window;
    auto* tabs = window.findChild<QTabWidget*>();
    if (!tabs || tabs->count() != 5) return 1;
    auto* general = window.findChild<GeneralTab*>();
    if (!general) return 1;
    for (auto* radio : general->findChildren<QRadioButton*>())
        if (radio->isEnabled()) return 1;
    if (!general->apply().isEmpty()) return 1;
    // Historical and damaged recovery files must survive reload/apply unchanged.
    if (!QDir().mkpath(Util::appConfigDir())) return 1;
    for (const QByteArray bytes : {QByteArray("broken JSON"),
            QByteArray(R"({"mode":"diagnostic","disabled_services":["example.service"]})")}) {
        QFile state(Util::appConfigDir() + "/state.json");
        if (!state.open(QIODevice::WriteOnly) || state.write(bytes) != bytes.size()) return 1;
        state.close();
        general->reload();
        if (!general->apply().isEmpty()) return 1;
        if (!state.open(QIODevice::ReadOnly) || state.readAll() != bytes) return 1;
        for (auto* radio : general->findChildren<QRadioButton*>())
            if (radio->isEnabled()) return 1;
    }

    if (Bootloader::detect().isLimine) {
        auto* boot = window.findChild<BootTab*>();
        if (!boot || !boot->apply().isEmpty()) return 1;
        for (auto* spin : boot->findChildren<QSpinBox*>())
            if (spin->isEnabled()) return 1;
        for (auto* check : boot->findChildren<QCheckBox*>())
            if (check->isEnabled()) return 1;
    }
    window.show();
    QTimer::singleShot(200, &app, &QCoreApplication::quit);
    const int result = app.exec();
    QTextStream(stdout) << "PASS five-tab UI and read-only boot/general guards\n";
    return result;
}

#include "generaltab.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QRadioButton>
#include <QSet>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <vector>

#include "autostart.h"
#include "lang.h"
#include "services.h"
#include "util.h"

namespace {

void disableAllAutostart(QJsonArray* saved)
{
    for (const AutoInfo& a : Autostart::list()) {
        if (a.enabled) {
            Autostart::setEnabled(a, false);
            saved->append(a.fileName);
        }
    }
}

void restoreAutostart(const QJsonArray& saved)
{
    QSet<QString> names;
    for (const QJsonValue& v : saved)
        names.insert(v.toString());
    for (const AutoInfo& a : Autostart::list()) {
        if (names.contains(a.fileName) && !a.enabled)
            Autostart::setEnabled(a, true);
    }
}

// 批量禁用可切换且不受保护的服务，返回实际禁用的清单
QStringList bulkDisableServices(int* protectedSkipped)
{
    QStringList units;
    int skipped = 0;
    for (const ServiceInfo& s : Services::list()) {
        if (!s.toggleable || !(s.state == "enabled" || s.state == "enabled-runtime"))
            continue;
        if (Services::isProtected(s.unit)) {
            ++skipped;
            continue;
        }
        units << s.unit;
    }
    if (protectedSkipped)
        *protectedSkipped = skipped;
    if (units.isEmpty())
        return {};
    QStringList quoted;
    for (const QString& u : units)
        quoted << Util::shellQuote(u);
    Util::runRoot("systemctl disable " + quoted.join(' '));
    return units;
}

void bulkEnableServices(const QStringList& units)
{
    if (units.isEmpty())
        return;
    QStringList quoted;
    for (const QString& u : units)
        quoted << Util::shellQuote(u);
    Util::runRoot("systemctl enable " + quoted.join(' '));
}

} // namespace

GeneralTab::GeneralTab(QWidget* parent)
    : ConfigTab(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(10);

    auto* modeBox = new QGroupBox(L("G_STARTUP_SELECTION"), this);
    auto* grid = new QGridLayout(modeBox);
    m_normal = new QRadioButton(L("G_NORMAL"), modeBox);
    m_diag = new QRadioButton(L("G_DIAG"), modeBox);
    m_sel = new QRadioButton(L("G_SELECTIVE"), modeBox);
    auto* descLabel = new QLabel(L("G_SEL_DESC"), modeBox);
    descLabel->setWordWrap(true);
    grid->addWidget(m_normal, 0, 0);
    grid->addWidget(m_diag, 1, 0);
    grid->addWidget(m_sel, 2, 0);
    grid->addWidget(descLabel, 0, 1, 3, 1);
    grid->setColumnStretch(1, 1);
    rootLayout->addWidget(modeBox);
    m_normal->setChecked(true);

    auto* selBox = new QGroupBox(this);
    auto* selLay = new QVBoxLayout(selBox);
    m_loadSvcs = new QCheckBox(L("G_LOAD_SVCS"), selBox);
    m_loadStartup = new QCheckBox(L("G_LOAD_STARTUP"), selBox);
    m_loadSvcs->setChecked(true);
    m_loadStartup->setChecked(true);
    selLay->addWidget(m_loadSvcs);
    selLay->addWidget(m_loadStartup);
    rootLayout->addWidget(selBox);
    connect(m_sel, &QRadioButton::toggled, selBox, &QGroupBox::setEnabled);

    m_note = new QLabel(L("G_NOTE"), this);
    m_note->setWordWrap(true);
    m_note->setStyleSheet("color:gray;");
    rootLayout->addWidget(m_note);
    rootLayout->addStretch(1);

    reload();
}

void GeneralTab::reload()
{
    QJsonObject st;
    QString err;
    if (Util::loadJson(&st, &err) && !err.isEmpty())
        m_note->setText(err);
    m_mode = st.value("mode").toString("normal");
    m_loadedSvc = st.value("load_system_services").toBool(true);
    m_loadedStartup = st.value("load_startup_items").toBool(true);

    m_normal->setChecked(m_mode == "normal");
    m_diag->setChecked(m_mode == "diagnostic");
    m_sel->setChecked(m_mode == "selective");
    m_loadSvcs->setChecked(m_loadedSvc);
    m_loadStartup->setChecked(m_loadedStartup);
}

void GeneralTab::setControlsEnabled(bool on)
{
    for (QWidget* w : std::vector<QWidget*>{m_normal, m_diag, m_sel, m_loadSvcs, m_loadStartup})
        w->setEnabled(on);
}

QStringList GeneralTab::apply()
{
    QStringList errs;
    const QString target = m_diag->isChecked()   ? "diagnostic"
                           : m_sel->isChecked()  ? "selective"
                                                 : "normal";
    const bool wantSvc = m_loadSvcs->isChecked();
    const bool wantStartup = m_loadStartup->isChecked();

    if (target == m_mode
        && (target != "selective"
            || (wantSvc == m_loadedSvc && wantStartup == m_loadedStartup)))
        return errs; // 无变化

    QJsonObject st;
    Util::loadJson(&st);
    QJsonArray savedSvcs = st.value("disabled_services").toArray();
    QJsonArray savedStartup = st.value("disabled_autostart").toArray();

    const bool toGraphical = (m_mode == "diagnostic" && target != "diagnostic");

    if (target == "diagnostic") {
        if (QMessageBox::question(this, L("MSG_CONFIRM"),
                                  L("CONF_DIAG").arg(L("G_DIAG_DESC")))
            != QMessageBox::Yes)
            return errs;
        {
            BusyCursor b;
            Util::runRoot("systemctl set-default multi-user.target");
            disableAllAutostart(&savedStartup);
        }
    } else {
        if (toGraphical) {
            BusyCursor b;
            Util::runRoot("systemctl set-default graphical.target");
        }
        const bool effStartup = (target == "normal") ? true : wantStartup;
        const bool effSvc = (target == "normal") ? true : wantSvc;

        if (!effStartup) {
            if (savedStartup.isEmpty()) {
                BusyCursor b;
                disableAllAutostart(&savedStartup);
            }
        } else if (!savedStartup.isEmpty()) {
            BusyCursor b;
            restoreAutostart(savedStartup);
            savedStartup = {};
        }

        if (!effSvc) {
            if (savedSvcs.isEmpty()) {
                int skipped = 0;
                // 先数一遍用于确认框
                int willDisable = 0;
                for (const ServiceInfo& s : Services::list()) {
                    if (s.toggleable && (s.state == "enabled" || s.state == "enabled-runtime")
                        && !Services::isProtected(s.unit))
                        ++willDisable;
                    else if (s.toggleable
                             && (s.state == "enabled" || s.state == "enabled-runtime"))
                        ++skipped;
                }
                if (QMessageBox::question(this, L("MSG_CONFIRM"),
                                          L("CONF_DIS_SVCS").arg(willDisable).arg(skipped))
                    != QMessageBox::Yes)
                    return errs;
                BusyCursor b;
                const QStringList units = bulkDisableServices(nullptr);
                for (const QString& u : units)
                    savedSvcs.append(u);
            }
        } else if (!savedSvcs.isEmpty()) {
            BusyCursor b;
            QStringList units;
            for (const QJsonValue& v : savedSvcs)
                units << v.toString();
            bulkEnableServices(units);
            savedSvcs = {};
        }
    }

    st.insert("mode", target);
    st.insert("load_system_services", target == "normal" ? true : wantSvc);
    st.insert("load_startup_items", target == "normal" ? true : wantStartup);
    st.insert("disabled_services", savedSvcs);
    st.insert("disabled_autostart", savedStartup);
    QString jerr;
    if (!Util::saveJson(st, &jerr))
        errs << jerr;

    m_mode = target;
    m_loadedSvc = st.value("load_system_services").toBool(true);
    m_loadedStartup = st.value("load_startup_items").toBool(true);
    return errs;
}

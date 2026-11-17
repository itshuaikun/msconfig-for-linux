#include "generaltab.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QRadioButton>
#include <QVBoxLayout>
#include <vector>

#include "lang.h"
#include "util.h"

namespace {
QString translated(const char* zh, const char* en)
{
    return QString::fromUtf8(QLocale::system().language() == QLocale::Chinese ? zh : en);
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
    auto* descLabel = new QLabel(translated(
        "此页仅显示本工具以前保存的选择，不代表当前系统的实际启动状态。",
        "This page shows this tool's previously saved selection, not the actual current system startup state."), modeBox);
    descLabel->setWordWrap(true);
    grid->addWidget(m_normal, 0, 0);
    grid->addWidget(m_diag, 1, 0);
    grid->addWidget(m_sel, 2, 0);
    grid->addWidget(descLabel, 0, 1, 3, 1);
    grid->setColumnStretch(1, 1);
    rootLayout->addWidget(modeBox);

    auto* selBox = new QGroupBox(this);
    auto* selLay = new QVBoxLayout(selBox);
    m_loadSvcs = new QCheckBox(L("G_LOAD_SVCS"), selBox);
    m_loadStartup = new QCheckBox(L("G_LOAD_STARTUP"), selBox);
    selLay->addWidget(m_loadSvcs);
    selLay->addWidget(m_loadStartup);
    rootLayout->addWidget(selBox);

    m_note = new QLabel(this);
    m_note->setWordWrap(true);
    m_note->setTextFormat(Qt::PlainText);
    rootLayout->addWidget(m_note);
    rootLayout->addStretch(1);

    reload();
}

void GeneralTab::reload()
{
    setControlsEnabled(false);
    // Clear any previously displayed selection before attempting to read the log.
    for (QRadioButton* button : {m_normal, m_diag, m_sel}) {
        button->setAutoExclusive(false);
        button->setChecked(false);
        button->setAutoExclusive(true);
    }
    m_loadSvcs->setChecked(false);
    m_loadStartup->setChecked(false);

    const QString explanation = translated(
        "批量启动模式暂为只读：旧版本未完整记录原始启动目标和服务状态，无法保证可靠恢复。应用或确定不会修改此页设置。",
        "Bulk startup modes are temporarily read-only: older versions did not record the original boot target and service states completely, so reliable restoration cannot be guaranteed. Apply and OK do not change these settings.");
    QJsonObject st;
    QString err;
    if (!Util::loadJson(&st, &err)) {
        m_note->setText(explanation + "\n\n" + translated(
            "无法读取恢复记录；文件将保留原样。错误：",
            "Cannot read the recovery record; the file is left unchanged. Error: ") + err);
        return;
    }
    if (st.isEmpty()) {
        m_note->setText(explanation + "\n\n" + translated(
            "没有可显示的历史启动记录。",
            "There is no saved startup record to display."));
        return;
    }

    const QString mode = st.value("mode").toString();
    m_normal->setChecked(mode == "normal");
    m_diag->setChecked(mode == "diagnostic");
    m_sel->setChecked(mode == "selective");
    m_loadSvcs->setChecked(st.value("load_system_services").toBool(false));
    m_loadStartup->setChecked(st.value("load_startup_items").toBool(false));
    const int services = st.value("disabled_services").toArray().size();
    const int startup = st.value("disabled_autostart").toArray().size();
    m_note->setText(explanation + "\n\n" + translated(
        "历史记录：模式 %1，服务 %2 项，自启动 %3 项。记录保留原样；如需恢复，请先核实当前状态与原设置，再逐项处理。",
        "Saved record: mode %1, %2 services, %3 autostart entries. The record is preserved. To restore, first verify current and original settings, then handle each item individually.")
        .arg(mode).arg(services).arg(startup));
}

void GeneralTab::setControlsEnabled(bool on)
{
    for (QWidget* w : std::vector<QWidget*>{m_normal, m_diag, m_sel, m_loadSvcs, m_loadStartup})
        w->setEnabled(on);
}

QStringList GeneralTab::apply()
{
    // Fail closed until a transactional recovery format can preserve exact prior
    // state and track each confirmed mutation. Never rewrite an old/damaged log.
    return {};
}

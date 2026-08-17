#include "servicestab.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "lang.h"
#include "mainwindow.h"
#include "util.h"

ServicesTab::ServicesTab(QWidget* parent)
    : ConfigTab(parent)
{
    auto* rootLayout = new QVBoxLayout(this);

    m_hideSystem = new QCheckBox(L("S_HIDE"), this);
    connect(m_hideSystem, &QCheckBox::toggled, this, &ServicesTab::onHideToggle);
    rootLayout->addWidget(m_hideSystem);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(4);
    m_tree->setHeaderLabels({L("COL_SERVICE"), L("COL_DESC"), L("COL_STATE"), L("COL_DATE")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    rootLayout->addWidget(m_tree, 1);

    auto* btnRow = new QHBoxLayout;
    m_enableBtn = new QPushButton(L("BTN_ENABLE"), this);
    m_disableBtn = new QPushButton(L("BTN_DISABLE"), this);
    m_enableAllBtn = new QPushButton(L("BTN_ENABLE_ALL"), this);
    m_disableAllBtn = new QPushButton(L("BTN_DISABLE_ALL"), this);
    m_refreshBtn = new QPushButton(L("BTN_REFRESH"), this);
    btnRow->addWidget(m_enableBtn);
    btnRow->addWidget(m_disableBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(m_enableAllBtn);
    btnRow->addWidget(m_disableAllBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(m_refreshBtn);
    rootLayout->addLayout(btnRow);

    connect(m_enableBtn, &QPushButton::clicked, this, &ServicesTab::onEnableSelected);
    connect(m_disableBtn, &QPushButton::clicked, this, &ServicesTab::onDisableSelected);
    connect(m_enableAllBtn, &QPushButton::clicked, this, &ServicesTab::onEnableAll);
    connect(m_disableAllBtn, &QPushButton::clicked, this, &ServicesTab::onDisableAll);
    connect(m_refreshBtn, &QPushButton::clicked, [this] { reload(); });

    reload();
}

static QString stateText(const QString& state)
{
    if (state == "enabled" || state == "enabled-runtime")
        return L("ST_ENABLED");
    if (state == "disabled")
        return L("ST_DISABLED");
    return state;
}

void ServicesTab::populate()
{
    m_tree->clear();
    QString err;
    m_rows = Services::list(&err);
    const bool hide = m_hideSystem->isChecked();

    if (m_rows.isEmpty()) {
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, err.isEmpty() ? L("S_NONE_FOUND") : err);
        return;
    }

    for (int i = 0; i < m_rows.size(); ++i) {
        const ServiceInfo& s = m_rows.at(i);
        if (hide && !s.userInstalled)
            continue;
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, s.unit);
        item->setData(0, Qt::UserRole, i);
        item->setText(1, s.desc);
        item->setText(2, stateText(s.state));
        item->setTextAlignment(2, Qt::AlignCenter);
        if (s.modified.isValid())
            item->setText(3, QLocale().toString(s.modified.date(), QLocale::ShortFormat));
        item->setToolTip(0, s.path);
        if (s.toggleable) {
            const bool active = (s.state == "enabled" || s.state == "enabled-runtime");
            item->setCheckState(0, active ? Qt::Checked : Qt::Unchecked);
        } else {
            // static/masked 等不可直接切换的条目：置灰并说明
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEnabled);
            item->setToolTip(2, s.state == "masked" ? L("ST_MASKED_TIP") : L("ST_STATIC_TIP"));
        }
    }
}

void ServicesTab::reload()
{
    populate();
}

void ServicesTab::onHideToggle()
{
    populate();
}

QStringList ServicesTab::applyUnits(const QStringList& units, bool enable)
{
    QStringList errs;
    if (units.isEmpty())
        return errs;
    QStringList quoted;
    for (const QString& u : units)
        quoted << Util::shellQuote(u);
    const QString cmd = "systemctl " + QString(enable ? "enable" : "disable") + " "
        + quoted.join(' ');
    Util::RunResult r;
    {
        BusyCursor b;
        r = Util::runRoot(cmd);
    }
    if (r.asyncLaunched) {
        QMessageBox::information(this, L("MSG_INFO"), L("MSG_ASYNC"));
        return errs;
    }
    if (!r.ok) {
        MainWindow::showOutput(this, L("MSG_PARTIAL"), r.output);
        errs << QString("systemctl %1 failed (%2)").arg(enable ? "enable" : "disable").arg(r.exitCode);
    }
    return errs;
}

void ServicesTab::onEnableSelected()
{
    if (m_rows.isEmpty())
        return;
    QStringList units;
    const QList<QTreeWidgetItem*> items = m_tree->selectedItems();
    for (QTreeWidgetItem* it : items) {
        const int i = it->data(0, Qt::UserRole).toInt();
        if (i >= 0 && i < m_rows.size() && m_rows.at(i).toggleable)
            units << m_rows.at(i).unit;
    }
    applyUnits(units, true);
    reload();
}

void ServicesTab::onDisableSelected()
{
    if (m_rows.isEmpty())
        return;
    QStringList units;
    const QList<QTreeWidgetItem*> items = m_tree->selectedItems();
    for (QTreeWidgetItem* it : items) {
        const int i = it->data(0, Qt::UserRole).toInt();
        if (i >= 0 && i < m_rows.size() && m_rows.at(i).toggleable)
            units << m_rows.at(i).unit;
    }
    applyUnits(units, false);
    reload();
}

void ServicesTab::onEnableAll()
{
    QStringList units;
    for (const ServiceInfo& s : m_rows) {
        if (s.toggleable && s.state == "disabled")
            units << s.unit;
    }
    applyUnits(units, true);
    reload();
}

void ServicesTab::onDisableAll()
{
    QStringList units;
    int skipped = 0;
    for (const ServiceInfo& s : m_rows) {
        if (!s.toggleable || !(s.state == "enabled" || s.state == "enabled-runtime"))
            continue;
        if (Services::isProtected(s.unit)) {
            ++skipped;
            continue;
        }
        units << s.unit;
    }
    if (units.isEmpty() && skipped == 0)
        return;
    if (QMessageBox::question(this, L("MSG_CONFIRM"), L("CONF_DIS_ALL_SVCS"))
        != QMessageBox::Yes)
        return;
    applyUnits(units, false);
    if (skipped > 0)
        QMessageBox::information(this, L("MSG_INFO"), L("S_PROTECTED_SKIP_FMT").arg(skipped));
    reload();
}

QStringList ServicesTab::apply()
{
    QStringList errs;
    if (m_rows.isEmpty())
        return errs;
    QStringList toEnable, toDisable;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* it = m_tree->topLevelItem(i);
        const int idx = it->data(0, Qt::UserRole).toInt();
        if (idx < 0 || idx >= m_rows.size())
            continue;
        const ServiceInfo& s = m_rows.at(idx);
        if (!s.toggleable)
            continue;
        const bool wasActive = (s.state == "enabled" || s.state == "enabled-runtime");
        const bool nowActive = (it->checkState(0) == Qt::Checked);
        if (nowActive != wasActive)
            (nowActive ? toEnable : toDisable) << s.unit;
    }
    errs << applyUnits(toEnable, true);
    errs << applyUnits(toDisable, false);
    const int n = toEnable.size() + toDisable.size();
    if (n > 0 && errs.isEmpty())
        QMessageBox::information(this, L("MSG_INFO"), L("S_TOGGLED_FMT").arg(n));
    reload();
    return errs;
}

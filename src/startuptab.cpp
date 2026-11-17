#include "startuptab.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "lang.h"

StartupTab::StartupTab(QWidget* parent)
    : ConfigTab(parent)
{
    auto* rootLayout = new QVBoxLayout(this);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(4);
    m_tree->setHeaderLabels({L("U_ITEM"), L("COL_CMD"), L("COL_LOCATION"), L("COL_STATUS")});
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
    m_refreshBtn = new QPushButton(L("BTN_REFRESH"), this);
    btnRow->addWidget(m_enableBtn);
    btnRow->addWidget(m_disableBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(m_refreshBtn);
    rootLayout->addLayout(btnRow);

    connect(m_enableBtn, &QPushButton::clicked, this, &StartupTab::onEnableSelected);
    connect(m_disableBtn, &QPushButton::clicked, this, &StartupTab::onDisableSelected);
    connect(m_refreshBtn, &QPushButton::clicked, [this] { reload(); });

    reload();
}

void StartupTab::reload()
{
    m_tree->clear();
    m_rows = Autostart::list();
    if (m_rows.isEmpty()) {
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, L("U_NONE_FOUND"));
        return;
    }
    for (int i = 0; i < m_rows.size(); ++i) {
        const AutoInfo& a = m_rows.at(i);
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, a.title);
        item->setData(0, Qt::UserRole, i);
        item->setCheckState(0, a.enabled ? Qt::Checked : Qt::Unchecked);
        item->setText(1, a.exec);
        item->setText(2, a.userPath.isEmpty() ? L("LOC_SYSTEM") : L("LOC_USER"));
        item->setText(3, a.enabled ? L("ST_ENABLED") : L("ST_DISABLED"));
        item->setTextAlignment(3, Qt::AlignCenter);
        if (!a.applicable) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsUserCheckable);
            item->setText(3, L("ST_NOT_APPLICABLE"));
        }
        QString tip = a.comment;
        if (!a.userPath.isEmpty())
            tip += "\n" + a.userPath;
        if (!a.systemPath.isEmpty())
            tip += "\n" + a.systemPath;
        item->setToolTip(0, tip);
    }
}

bool StartupTab::applyUnits(const QStringList& fileNames, bool enable)
{
    bool allOk = true;
    int n = 0;
    for (const AutoInfo& a : m_rows) {
        if (!fileNames.contains(a.fileName))
            continue;
        QString err;
        if (Autostart::setEnabled(a, enable, &err)) {
            ++n;
        } else {
            allOk = false;
            QMessageBox::warning(this, L("MSG_ERROR"), a.fileName + "\n" + err);
        }
    }
    if (n > 0 && allOk)
        QMessageBox::information(this, L("MSG_INFO"), L("U_DONE_FMT").arg(n));
    return allOk;
}

void StartupTab::onEnableSelected()
{
    if (m_rows.isEmpty())
        return;
    QStringList names;
    const QList<QTreeWidgetItem*> items = m_tree->selectedItems();
    for (QTreeWidgetItem* it : items) {
        const int idx = it->data(0, Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_rows.size())
            names << m_rows.at(idx).fileName;
    }
    applyUnits(names, true);
    reload();
}

void StartupTab::onDisableSelected()
{
    if (m_rows.isEmpty())
        return;
    QStringList names;
    const QList<QTreeWidgetItem*> items = m_tree->selectedItems();
    for (QTreeWidgetItem* it : items) {
        const int idx = it->data(0, Qt::UserRole).toInt();
        if (idx >= 0 && idx < m_rows.size())
            names << m_rows.at(idx).fileName;
    }
    applyUnits(names, false);
    reload();
}

QStringList StartupTab::apply()
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
        const AutoInfo& a = m_rows.at(idx);
        const bool nowActive = (it->checkState(0) == Qt::Checked);
        if (nowActive != a.enabled)
            (nowActive ? toEnable : toDisable) << a.fileName;
    }
    const int n = toEnable.size() + toDisable.size();
    if (n > 0) {
        if (!applyUnits(toEnable, true)) errs << L("MSG_PARTIAL");
        if (!applyUnits(toDisable, false)) errs << L("MSG_PARTIAL");
        reload();
    }
    return errs;
}

#include "boottab.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTextStream>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "lang.h"
#include "mainwindow.h"
#include "util.h"

namespace {

// 在原始内核参数串上应用三个开关（保留其余参数不动）
QString mergeCmdline(const QString& raw, bool quiet, bool splash, bool safeBoot)
{
    QStringList out;
    for (const QString& tok : raw.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts)) {
        if (tok == "quiet" || tok == "splash" || tok == "systemd.unit=multi-user.target")
            continue;
        out << tok;
    }
    if (safeBoot)
        out << "systemd.unit=multi-user.target";
    if (quiet)
        out << "quiet";
    if (splash)
        out << "splash";
    return out.join(' ');
}

} // namespace

BootTab::BootTab(QWidget* parent)
    : ConfigTab(parent)
{
    auto* rootLayout = new QVBoxLayout(this);

    m_noGrubLabel = new QLabel(L("B_NOGRUB"), this);
    m_noGrubLabel->setWordWrap(true);
    m_noGrubLabel->setStyleSheet("color:#b00; padding:8px;");
    m_noGrubLabel->hide();
    rootLayout->addWidget(m_noGrubLabel);

    auto* listLabel = new QLabel(L("B_ENTRIES"), this);
    rootLayout->addWidget(listLabel);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({L("B_ENTRIES"), L("COL_STATE")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            this, &BootTab::onItemDoubleClicked);
    rootLayout->addWidget(m_tree, 1);

    m_defaultLabel = new QLabel(this);
    rootLayout->addWidget(m_defaultLabel);
    auto* hint = new QLabel(L("B_HINT"), this);
    hint->setStyleSheet("color:gray;");
    rootLayout->addWidget(hint);

    auto* btnRow = new QHBoxLayout;
    m_setDefaultBtn = new QPushButton(L("B_SETDEFAULT"), this);
    m_advancedBtn = new QPushButton(L("B_ADVANCED"), this);
    m_regenBtn = new QPushButton(L("B_REGEN"), this);
    btnRow->addWidget(m_setDefaultBtn);
    btnRow->addWidget(m_advancedBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(m_regenBtn);
    rootLayout->addLayout(btnRow);
    connect(m_setDefaultBtn, &QPushButton::clicked, this, &BootTab::onSetDefault);
    connect(m_advancedBtn, &QPushButton::clicked, this, &BootTab::onAdvanced);
    connect(m_regenBtn, &QPushButton::clicked, this, &BootTab::onRegen);

    auto* options = new QGroupBox(L("B_OPTIONS"), this);
    auto* optLayout = new QGridLayout(options);
    m_quiet = new QCheckBox(L("B_QUIET"), options);
    m_splash = new QCheckBox(L("B_SPLASH"), options);
    m_safeBoot = new QCheckBox(L("B_SAFEBOOT"), options);
    auto* timeoutLabel = new QLabel(L("B_TIMEOUT"), options);
    m_timeout = new QSpinBox(options);
    m_timeout->setRange(0, 99);
    auto* secondsLabel = new QLabel(L("B_SECONDS"), options);
    optLayout->addWidget(m_quiet, 0, 0, 1, 4);
    optLayout->addWidget(m_splash, 1, 0, 1, 4);
    optLayout->addWidget(m_safeBoot, 2, 0, 1, 4);
    optLayout->addWidget(timeoutLabel, 3, 0);
    optLayout->addWidget(m_timeout, 3, 1);
    optLayout->addWidget(secondsLabel, 3, 2);
    optLayout->setColumnStretch(3, 1);
    rootLayout->addWidget(options);

    reload();
}

void BootTab::reload()
{
    const QString cfg = Grub::cfgPath();
    m_hasGrub = !cfg.isEmpty() && QFile::exists(Grub::defaultsPath());
    const bool enable = m_hasGrub;
    m_tree->setEnabled(enable);
    m_setDefaultBtn->setEnabled(enable);
    m_advancedBtn->setEnabled(enable);
    m_regenBtn->setEnabled(enable);
    m_quiet->setEnabled(enable);
    m_splash->setEnabled(enable);
    m_safeBoot->setEnabled(enable);
    m_timeout->setEnabled(enable);
    m_noGrubLabel->setVisible(!enable);
    if (!enable) {
        m_tree->clear();
        m_defaultLabel->clear();
        return;
    }

    m_entries = Grub::parseMenu(cfg);
    const QHash<QString, QString> defs = Grub::readDefaults();
    m_cmdlineDefault = defs.value("GRUB_CMDLINE_LINUX_DEFAULT");
    m_cmdlineAll = defs.value("GRUB_CMDLINE_LINUX");
    m_currentDefault = resolveCurrentDefault(defs);
    m_chosenDefault.clear();

    bool ok = false;
    const int t = defs.value("GRUB_TIMEOUT", "5").toInt(&ok);
    m_timeout->setValue(ok ? t : 5);
    syncCheckboxesFromCmdline();
    populateTree();
}

void BootTab::syncCheckboxesFromCmdline()
{
    const QStringList toks = m_cmdlineDefault.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    m_quiet->setChecked(toks.contains("quiet"));
    m_splash->setChecked(toks.contains("splash"));
    m_safeBoot->setChecked(toks.contains("systemd.unit=multi-user.target"));
}

void BootTab::populateTree()
{
    m_tree->clear();
    for (const GrubEntry& e : m_entries) {
        auto* item = new QTreeWidgetItem(m_tree);
        const bool isDefault = (e.path == m_currentDefault);
        item->setText(0, QString(e.depth * 3, QChar(' ')) + e.title);
        item->setText(1, e.isSubmenu ? L("B_TYPE_SUB") : L("B_TYPE_ENTRY"));
        item->setData(0, Qt::UserRole, e.path);
        QFont f = item->font(0);
        f.setBold(isDefault);
        item->setFont(0, f);
        if (isDefault)
            item->setText(0, "● " + item->text(0));
    }
    m_tree->expandAll();
    QString def = m_chosenDefault.isEmpty() ? m_currentDefault : m_chosenDefault;
    if (def.isEmpty() || def == "saved")
        def = L("B_NONE");
    m_defaultLabel->setText(L("B_CURRENT_FMT").arg(def));
}

QString BootTab::resolveCurrentDefault(const QHash<QString, QString>& defs)
{
    QString raw = defs.value("GRUB_DEFAULT").trimmed();
    if (raw == "saved") {
        QProcess p;
        p.start("grub-editenv", {"list"});
        if (p.waitForFinished(3000)) {
            const QStringList lines = QString::fromUtf8(p.readAllStandardOutput()).split('\n');
            for (const QString& l : lines) {
                if (l.startsWith("saved_entry="))
                    raw = l.mid(12).trimmed();
            }
        }
    }
    bool ok = false;
    const int idx = raw.toInt(&ok);
    if (ok && idx >= 0) {
        int top = 0;
        for (const GrubEntry& e : m_entries) {
            if (e.depth == 0) {
                if (top == idx)
                    return e.path;
                ++top;
            }
        }
        return raw; // 序号越界（如条目已变化），原样显示
    }
    for (const GrubEntry& e : m_entries) {
        if (e.path == raw || e.title == raw)
            return e.path;
    }
    return raw;
}

void BootTab::onItemDoubleClicked(QTreeWidgetItem*, int)
{
    onSetDefault();
}

void BootTab::onSetDefault()
{
    QTreeWidgetItem* item = m_tree->currentItem();
    if (!item)
        return;
    m_chosenDefault = item->data(0, Qt::UserRole).toString();
    // 重新标注
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* it = m_tree->topLevelItem(i);
        const bool chosen = it->data(0, Qt::UserRole).toString() == m_chosenDefault;
        QFont f = it->font(0);
        f.setBold(chosen);
        it->setFont(0, f);
        QString text = it->text(0);
        if (chosen && !text.startsWith("● "))
            it->setText(0, "● " + text);
        else if (!chosen && text.startsWith("● "))
            it->setText(0, text.mid(2));
    }
    m_defaultLabel->setText(L("B_CURRENT_FMT").arg(m_chosenDefault));
}

void BootTab::onAdvanced()
{
    QDialog dlg(this);
    dlg.setWindowTitle(L("B_ADV_TITLE"));
    dlg.resize(560, 380);
    auto* lay = new QVBoxLayout(&dlg);
    auto* l1 = new QLabel(L("B_CMDLINE_DEFAULT_L"), &dlg);
    auto* e1 = new QPlainTextEdit(m_cmdlineDefault, &dlg);
    auto* l2 = new QLabel(L("B_CMDLINE_ALL_L"), &dlg);
    auto* e2 = new QPlainTextEdit(m_cmdlineAll, &dlg);
    auto* hint = new QLabel(L("B_CMDLINE_HINT"), &dlg);
    hint->setStyleSheet("color:gray;");
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(l1);
    lay->addWidget(e1, 1);
    lay->addWidget(l2);
    lay->addWidget(e2, 1);
    lay->addWidget(hint);
    lay->addWidget(bb);
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_cmdlineDefault = e1->toPlainText().simplified();
    m_cmdlineAll = e2->toPlainText().simplified();
    syncCheckboxesFromCmdline();
}

void BootTab::onRegen()
{
    const QString cfg = Grub::cfgPath();
    if (cfg.isEmpty())
        return;
    QString cmd = "test -e " + Util::shellQuote(cfg + ".msconfig-bak")
        + " || cp -a " + Util::shellQuote(cfg) + " " + Util::shellQuote(cfg + ".msconfig-bak")
        + "; " + Grub::mkconfigCommand();
    Util::RunResult r;
    {
        BusyCursor b;
        r = Util::runRoot(cmd);
    }
    if (r.asyncLaunched) {
        QMessageBox::information(this, L("MSG_INFO"), L("MSG_ASYNC"));
    } else {
        MainWindow::showOutput(this, L("B_OUT_TITLE"), r.output);
        if (r.ok)
            QMessageBox::information(this, L("MSG_INFO"), L("B_REGEN_DONE"));
    }
    reload();
}

QStringList BootTab::apply()
{
    QStringList errs;
    if (!m_hasGrub)
        return errs;

    const QString merged = mergeCmdline(m_cmdlineDefault, m_quiet->isChecked(),
                                        m_splash->isChecked(), m_safeBoot->isChecked());
    const QHash<QString, QString> defs = Grub::readDefaults();

    bool changed = false;
    if (m_timeout->text().toInt() != defs.value("GRUB_TIMEOUT").toInt())
        changed = true;
    if (merged != defs.value("GRUB_CMDLINE_LINUX_DEFAULT"))
        changed = true;
    if (m_cmdlineAll != defs.value("GRUB_CMDLINE_LINUX"))
        changed = true;
    if (!m_chosenDefault.isEmpty() && m_chosenDefault != m_currentDefault)
        changed = true;
    if (!changed)
        return errs;

    QHash<QString, QString> kv;
    kv.insert("GRUB_TIMEOUT", QString::number(m_timeout->value()));
    kv.insert("GRUB_CMDLINE_LINUX_DEFAULT", Grub::quoteValue(merged));
    if (m_cmdlineAll != defs.value("GRUB_CMDLINE_LINUX"))
        kv.insert("GRUB_CMDLINE_LINUX", Grub::quoteValue(m_cmdlineAll));
    if (!m_chosenDefault.isEmpty())
        kv.insert("GRUB_DEFAULT", Grub::quoteValue(m_chosenDefault));

    QFile oldFile(Grub::defaultsPath());
    if (!oldFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errs << oldFile.errorString();
        return errs;
    }
    const QString newText = Grub::updatedDefaultsText(
        QString::fromUtf8(oldFile.readAll()), kv);
    oldFile.close();

    const QString tmp = QString("/tmp/msconfig-grub-%1").arg(QCoreApplication::applicationPid());
    {
        QFile tf(tmp);
        if (!tf.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            errs << tf.errorString();
            return errs;
        }
        tf.write(newText.toUtf8());
    }

    const QString def = Grub::defaultsPath();
    const QString cfg = Grub::cfgPath();
    QString cmd = "test -e " + Util::shellQuote(def + ".msconfig-bak")
        + " || cp -a " + Util::shellQuote(def) + " " + Util::shellQuote(def + ".msconfig-bak")
        + "; test -e " + Util::shellQuote(cfg + ".msconfig-bak")
        + " || cp -a " + Util::shellQuote(cfg) + " " + Util::shellQuote(cfg + ".msconfig-bak")
        + "; cp " + Util::shellQuote(tmp) + " " + Util::shellQuote(def)
        + " && chmod 644 " + Util::shellQuote(def)
        + "; " + Grub::mkconfigCommand();

    Util::RunResult r;
    {
        BusyCursor b;
        r = Util::runRoot(cmd);
    }
    QFile::remove(tmp);

    if (r.asyncLaunched) {
        QMessageBox::information(this, L("MSG_INFO"), L("MSG_ASYNC"));
    } else {
        MainWindow::showOutput(this, L("B_OUT_TITLE"), r.output);
        if (!r.ok) {
            errs << L("MSG_PARTIAL") + "\n" + r.error;
        } else {
            QMessageBox::information(this, L("MSG_INFO"),
                L("B_APPLIED") + "\n"
                + (!m_chosenDefault.isEmpty()
                       ? L("B_DEFAULT_SET").arg(m_chosenDefault)
                       : QString()));
        }
    }
    reload();
    return errs;
}

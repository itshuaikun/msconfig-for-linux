#include "mainwindow.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include "boottab.h"
#include "generaltab.h"
#include "grub.h"
#include "lang.h"
#include "servicestab.h"
#include "startuptab.h"
#include "toolstab.h"
#include "util.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(L("APP_TITLE"));
    resize(780, 580);

    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);

    m_tabs = new QTabWidget(central);
    m_general = new GeneralTab(this);
    m_boot = new BootTab(this);
    m_services = new ServicesTab(this);
    m_startup = new StartupTab(this);
    m_tools = new ToolsTab(this);
    m_tabs->addTab(m_general, L("TAB_GENERAL"));
    m_tabs->addTab(m_boot, L("TAB_BOOT"));
    m_tabs->addTab(m_services, L("TAB_SERVICES"));
    m_tabs->addTab(m_startup, L("TAB_STARTUP"));
    m_tabs->addTab(m_tools, L("TAB_TOOLS"));
    rootLayout->addWidget(m_tabs, 1);

    // 底部按钮行（msconfig 布局：确定 取消 应用 帮助）
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch(1);
    auto* okBtn = new QPushButton(L("BTN_OK"), central);
    okBtn->setDefault(true);
    auto* cancelBtn = new QPushButton(L("BTN_CANCEL"), central);
    auto* applyBtn = new QPushButton(L("BTN_APPLY"), central);
    auto* helpBtn = new QPushButton(L("BTN_HELP"), central);
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(applyBtn);
    btnRow->addWidget(helpBtn);
    rootLayout->addLayout(btnRow);
    setCentralWidget(central);

    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::onApply);
    connect(okBtn, &QPushButton::clicked, this, &MainWindow::onOk);
    connect(cancelBtn, &QPushButton::clicked, this, &MainWindow::close);
    connect(helpBtn, &QPushButton::clicked, this, &MainWindow::onHelp);

    const QString cfg = Grub::cfgPath();
    statusBar()->showMessage(L("STATUS_FMT")
                                 .arg(Util::distroName(), Util::sessionInfo(),
                                      Util::initSystem(),
                                      cfg.isEmpty() ? "-" : QFileInfo(cfg).fileName()));
}

void MainWindow::showOutput(QWidget* parent, const QString& title, const QString& text)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.resize(680, 440);
    auto* lay = new QVBoxLayout(&dlg);
    auto* te = new QPlainTextEdit(&dlg);
    te->setReadOnly(true);
    te->setPlainText(text.isEmpty() ? "(no output)" : text);
    lay->addWidget(te);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    // Close 按钮可能触发 accepted 或 rejected，两种都连上
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    lay->addWidget(bb);
    dlg.exec();
}

void MainWindow::onApply()
{
    QStringList errs;
    errs << m_general->apply();
    errs << m_boot->apply();
    errs << m_services->apply();
    errs << m_startup->apply();
    errs << m_tools->apply();
    errs.removeAll(QString());
    if (!errs.isEmpty())
        QMessageBox::warning(this, L("MSG_ERROR"), errs.join('\n'));
    else
        statusBar()->showMessage(L("MSG_DONE"), 4000);
}

void MainWindow::onOk()
{
    onApply();
    close();
}

void MainWindow::onHelp()
{
    QDialog dlg(this);
    dlg.setWindowTitle(L("HELP_TITLE"));
    dlg.resize(620, 480);
    auto* lay = new QVBoxLayout(&dlg);
    auto* te = new QPlainTextEdit(&dlg);
    te->setReadOnly(true);
    te->setPlainText(L("HELP_TEXT"));
    lay->addWidget(te);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    lay->addWidget(bb);
    dlg.exec();
}

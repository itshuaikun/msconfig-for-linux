#include "toolstab.h"

#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "lang.h"
#include "util.h"

namespace {
struct ToolDef {
    const char* nameKey;
    const char* descKey;
    const char* cmd;
};

// 全部为 POSIX sh 兼容脚本，按存在性自动回退，适配任意发行版
static const ToolDef kTools[] = {
    {"TOOL_SYSINFO", "TOOL_SYSINFO_D",
     "fastfetch 2>/dev/null || neofetch 2>/dev/null || inxi -Fz 2>/dev/null || uname -a"},
    {"TOOL_TASKMGR", "TOOL_TASKMGR_D",
     "htop 2>/dev/null || top"},
    {"TOOL_SVCLIST", "TOOL_SVCLIST_D",
     "systemctl list-units --type=service --no-pager"},
    {"TOOL_DISK", "TOOL_DISKINFO_D",
     "lsblk -f; echo; df -h"},
    {"TOOL_NET", "TOOL_NET_D",
     "ip -br addr; echo; ip route"},
    {"TOOL_LOG", "TOOL_LOG_D",
     "journalctl -b"},
    {"TOOL_PKGS", "TOOL_PKGS_D",
     "if command -v pacman >/dev/null 2>&1; then pacman -Q | less; "
     "elif command -v apt >/dev/null 2>&1; then dpkg -l | less; "
     "elif command -v dnf >/dev/null 2>&1; then dnf list installed | less; "
     "elif command -v zypper >/dev/null 2>&1; then zypper search -i | less; "
     "elif command -v apk >/dev/null 2>&1; then apk info | less; "
     "elif command -v xbps-query >/dev/null 2>&1; then xbps-query -l | less; "
     "else echo unknown package manager; fi"},
    {"TOOL_GRUBENV", "TOOL_GRUBENV_D",
     "grub-editenv list 2>/dev/null || sudo grub-editenv list"},
    {"TOOL_USER", "TOOL_USER_D",
     "id; echo; groups"},
};
} // namespace

ToolsTab::ToolsTab(QWidget* parent)
    : ConfigTab(parent)
{
    auto* rootLayout = new QVBoxLayout(this);

    m_list = new QListWidget(this);
    for (const ToolDef& t : kTools)
        m_list->addItem(L(t.nameKey));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    rootLayout->addWidget(m_list, 1);

    auto* detailLabel = new QLabel(L("T_DETAILS"), this);
    rootLayout->addWidget(detailLabel);
    m_detail = new QPlainTextEdit(this);
    m_detail->setReadOnly(true);
    m_detail->setMaximumHeight(88);
    rootLayout->addWidget(m_detail);

    auto* btnRow = new QVBoxLayout;
    m_launchBtn = new QPushButton(L("T_LAUNCH"), this);
    btnRow->addWidget(m_launchBtn);
    rootLayout->addLayout(btnRow);

    connect(m_list, &QListWidget::currentRowChanged, this, &ToolsTab::onSelectionChanged);
    connect(m_launchBtn, &QPushButton::clicked, this, &ToolsTab::onLaunch);

    m_list->setCurrentRow(0);
}

void ToolsTab::onSelectionChanged()
{
    const int row = m_list->currentRow();
    if (row < 0)
        return;
    m_detail->setPlainText(QString::fromLatin1(kTools[row].cmd));
}

void ToolsTab::onLaunch()
{
    const int row = m_list->currentRow();
    if (row < 0)
        return;
    if (!Util::launchTerminalScript(QString::fromLatin1(kTools[row].cmd)))
        QMessageBox::warning(this, L("MSG_ERROR"), L("T_NO_TERM"));
}

QStringList ToolsTab::apply()
{
    return {};
}

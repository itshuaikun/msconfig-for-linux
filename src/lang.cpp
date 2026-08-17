#include "lang.h"

#include <QLocale>
#include <QProcessEnvironment>
#include <QStringList>
#include <cstring>

namespace {
struct Ent {
    const char* key;
    const char* zh;
    const char* en;
};

static const Ent kTable[] = {
    {"APP_TITLE", "系统配置", "System Configuration"},
    {"STATUS_FMT", "发行版: %1   |   会话: %2   |   初始化: %3   |   引导: %4",
     "Distro: %1 | Session: %2 | Init: %3 | Bootloader: %4"},

    {"TAB_GENERAL", "常规", "General"},
    {"TAB_BOOT", "引导", "Boot"},
    {"TAB_SERVICES", "服务", "Services"},
    {"TAB_STARTUP", "启动", "Startup"},
    {"TAB_TOOLS", "工具", "Tools"},

    {"BTN_OK", "确定", "OK"},
    {"BTN_CANCEL", "取消", "Cancel"},
    {"BTN_APPLY", "应用", "Apply"},
    {"BTN_HELP", "帮助", "Help"},

    // ---- 常规 ----
    {"G_STARTUP_SELECTION", "启动选择", "Startup selection"},
    {"G_NORMAL", "正常启动", "Normal startup"},
    {"G_DIAG", "诊断启动", "Diagnostic startup"},
    {"G_SELECTIVE", "有选择的启动", "Selective startup"},
    {"G_SEL_DESC", "加载基本设备和服务，并有选择地加载其他项目：",
     "Load basic devices and services, but selectively load other components:"},
    {"G_LOAD_SVCS", "加载系统服务", "Load system services"},
    {"G_LOAD_STARTUP", "加载启动项", "Load startup items"},
    {"G_DIAG_DESC", "下次启动将进入无图形界面的多用户模式（类似 Windows 安全模式的基础环境）。",
     "Next boot will enter multi-user mode without a graphical UI (like a basic Windows safe mode)."},
    {"G_NOTE", "提示：此处更改即时写入 systemd 默认目标与自启动项；在“服务/启动”标签页可逐项调整。",
     "Note: changes here write the systemd default target and autostart items; use the Services/Startup tabs for per-item control."},

    // ---- 引导 ----
    {"B_ENTRIES", "引导项目：", "Boot entries:"},
    {"B_CURRENT_FMT", "当前默认：%1", "Current default: %1"},
    {"B_NONE", "（未识别）", "(not detected)"},
    {"B_SETDEFAULT", "设为默认", "Set as default"},
    {"B_ADVANCED", "高级选项...", "Advanced options..."},
    {"B_REGEN", "重新生成 GRUB 配置", "Regenerate GRUB config"},
    {"B_OPTIONS", "启动选项", "Boot options"},
    {"B_QUIET", "安静模式（quiet，隐藏内核启动消息）", "Quiet mode (quiet, hide kernel messages)"},
    {"B_SPLASH", "启动画面（splash）", "Splash screen (splash)"},
    {"B_SAFEBOOT", "安全引导 —— 无图形界面（systemd.unit=multi-user.target）",
     "Safe boot - no graphical UI (systemd.unit=multi-user.target)"},
    {"B_TIMEOUT", "超时：", "Timeout:"},
    {"B_SECONDS", "秒", "seconds"},
    {"B_NOGRUB", "未检测到 GRUB 配置文件（/boot/grub/grub.cfg 或 /boot/grub2/grub.cfg）。\n"
                 "本工具的“引导”页仅支持 GRUB；如果你使用 systemd-boot / Limine / rEFInd，请使用对应的官方工具。",
     "GRUB configuration not found (/boot/grub/grub.cfg or /boot/grub2/grub.cfg).\n"
     "The Boot tab only supports GRUB; for systemd-boot / Limine / rEFInd please use their own tools."},
    {"B_HINT", "提示：双击条目可直接设为默认引导项。", "Hint: double-click an entry to set it as the default."},
    {"B_ADV_TITLE", "引导高级选项", "Boot advanced options"},
    {"B_CMDLINE_DEFAULT_L", "其他内核参数（GRUB_CMDLINE_LINUX_DEFAULT）：",
     "Other kernel parameters (GRUB_CMDLINE_LINUX_DEFAULT):"},
    {"B_CMDLINE_ALL_L", "所有内核参数（GRUB_CMDLINE_LINUX，含恢复模式）：",
     "All kernel parameters (GRUB_CMDLINE_LINUX, incl. recovery):"},
    {"B_CMDLINE_HINT", "多个参数用空格分隔，例如：loglevel=3 nowatchdog",
     "Separate parameters with spaces, e.g. loglevel=3 nowatchdog"},
    {"B_OUT_TITLE", "GRUB 配置生成输出", "GRUB configuration output"},
    {"B_TYPE_SUB", "子菜单", "Submenu"},
    {"B_TYPE_ENTRY", "启动项", "Entry"},
    {"MSG_ASYNC", "系统未安装 polkit，已改为在终端中通过 sudo 执行，请在弹出的终端里完成操作。",
     "polkit is not installed; the command was launched with sudo in a terminal. Complete it there."},
    {"B_APPLIED", "引导设置已应用，GRUB 配置已重新生成。", "Boot settings applied, GRUB config regenerated."},
    {"B_REGEN_DONE", "GRUB 配置已重新生成。", "GRUB config regenerated."},
    {"B_DEFAULT_SET", "已将“%1”设为默认引导项。", "Set \"%1\" as the default boot entry."},

    // ---- 服务 ----
    {"S_HIDE", "隐藏系统自带服务（仅显示第三方/手动安装的服务）",
     "Hide system-bundled services (show only third-party / manually installed)"},
    {"S_NONE_FOUND", "未找到服务。systemd 是否在运行？", "No services found. Is systemd running?"},
    {"COL_SERVICE", "服务", "Service"},
    {"COL_DESC", "描述", "Description"},
    {"COL_STATE", "状态", "State"},
    {"COL_DATE", "日期", "Date"},
    {"COL_CMD", "命令", "Command"},
    {"COL_LOCATION", "位置", "Location"},
    {"COL_STATUS", "状态", "Status"},
    {"ST_ENABLED", "已启用", "Enabled"},
    {"ST_DISABLED", "已禁用", "Disabled"},
    {"ST_STATIC_TIP", "该服务未定义安装信息（static），由依赖关系自动触发，无法直接启用/禁用。",
     "This unit is static (no [Install]); it is pulled in by dependencies and cannot be enabled/disabled directly."},
    {"ST_MASKED_TIP", "该服务已被屏蔽（masked）。", "This unit is masked."},
    {"S_TOGGLED_FMT", "已处理 %1 项服务。", "%1 service operations applied."},
    {"CONF_DIS_ALL_SVCS", "即将禁用列表中全部可切换的服务（受保护的关键服务会自动跳过），可能影响系统功能。确定继续？",
     "This will disable every toggleable service in the list (critical services are protected). Continue?"},
    {"S_PROTECTED_SKIP_FMT", "已跳过受保护服务 %1 项。", "%1 protected services were skipped."},

    // ---- 启动 ----
    {"U_ITEM", "启动项", "Startup item"},
    {"U_NONE_FOUND", "未发现自启动项。", "No autostart items found."},
    {"LOC_USER", "用户", "User"},
    {"LOC_SYSTEM", "系统", "System"},
    {"U_DONE_FMT", "已处理 %1 个启动项。", "%1 startup items updated."},

    // ---- 工具 ----
    {"T_LAUNCH", "启动", "Launch"},
    {"T_DETAILS", "所选工具的命令：", "Command of the selected tool:"},
    {"T_NO_TERM", "未找到可用的终端模拟器（konsole/gnome-terminal/kitty/alacritty/foot/xterm 等），无法运行该工具。",
     "No terminal emulator found (konsole/gnome-terminal/kitty/alacritty/foot/xterm...). Cannot run this tool."},
    {"TOOL_SYSINFO", "系统信息", "System information"},
    {"TOOL_SYSINFO_D", "查看内核、发行版、硬件等综合信息", "Show kernel, distro and hardware information"},
    {"TOOL_TASKMGR", "任务管理器（进程）", "Task manager (processes)"},
    {"TOOL_TASKMGR_D", "htop/top 查看并管理运行中的进程", "View and manage running processes with htop/top"},
    {"TOOL_SVCLIST", "服务列表", "Service list"},
    {"TOOL_SVCLIST_D", "查看 systemd 全部服务的运行状态", "Show running state of all systemd services"},
    {"TOOL_DISK", "磁盘信息", "Disk information"},
    {"TOOL_DISKINFO_D", "查看块设备、文件系统与空间占用", "Show block devices, filesystems and disk usage"},
    {"TOOL_NET", "网络信息", "Network information"},
    {"TOOL_NET_D", "查看网卡地址与路由表", "Show network interfaces and routing table"},
    {"TOOL_LOG", "本次启动日志", "This boot's log"},
    {"TOOL_LOG_D", "查看本次开机的 journalctl 日志", "Show this boot's journalctl log"},
    {"TOOL_PKGS", "已安装软件包", "Installed packages"},
    {"TOOL_PKGS_D", "按发行版列出已安装的软件包", "List installed packages (distro-aware)"},
    {"TOOL_GRUBENV", "GRUB 环境变量", "GRUB environment"},
    {"TOOL_GRUBENV_D", "查看 grub-editenv 保存的默认引导等变量", "Show grub-editenv variables (saved default etc.)"},
    {"TOOL_USER", "用户与组", "Users and groups"},
    {"TOOL_USER_D", "查看当前用户身份与所属组", "Show current user identity and groups"},

    // ---- 通用消息 ----
    {"MSG_ROOT_CANCELLED", "授权已取消，操作未执行。", "Authorization cancelled, nothing was changed."},
    {"MSG_NO_PRIV", "未找到 pkexec，也没有可用的终端来运行 sudo。请安装 polkit（pkexec）或以 root 身份运行本程序。",
     "pkexec not found and no terminal available for sudo. Install polkit (pkexec) or run this program as root."},
    {"MSG_DONE", "操作已完成。", "All operations completed."},
    {"MSG_PARTIAL", "部分操作失败，输出如下：", "Some operations failed. Output:"},
    {"MSG_CONFIRM", "确认", "Confirm"},
    {"MSG_ERROR", "错误", "Error"},
    {"MSG_INFO", "提示", "Information"},
    {"CONF_DIAG", "切换到“诊断启动”？\n\n%1\n\n之后仍可在本工具中改回“正常启动”。",
     "Switch to Diagnostic startup?\n\n%1\nYou can switch back to Normal startup here later."},
    {"CONF_DIS_SVCS", "即将禁用 %1 个非关键系统服务（自动跳过受保护服务 %2 个）。\n"
                     "这可能影响声音、蓝牙、打印机等功能，且需要管理员授权。确定继续？",
     "About to disable %1 non-critical services (%2 protected ones will be skipped).\n"
     "This may affect sound, bluetooth, printing, etc. Continue?"},

    // ---- 帮助 ----
    {"HELP_TITLE", "关于 / 帮助", "About / Help"},
    {"HELP_TEXT",
     "msconfig for Linux —— Windows「系统配置(msconfig)」的 Linux 复刻（C++/Qt）。\n\n"
     "• 常规：正常/诊断/有选择的启动（systemd 默认目标 + 批量开关）\n"
     "• 引导：管理 GRUB 默认启动项、超时、内核参数（写入 /etc/default/grub 并重新生成 grub.cfg）\n"
     "• 服务：启用/禁用 systemd 服务（下次启动生效，与 msconfig 一致）\n"
     "• 启动：XDG 标准自启动项（~/.config/autostart 与 /etc/xdg/autostart）\n"
     "• 工具：常用系统工具快捷入口\n\n"
     "提权方式自动适配：root 直接执行 → pkexec → 终端内 sudo。\n"
     "适用于任意发行版（Arch/Debian/Fedora/openSUSE...）与任意 DE/WM（X11/Wayland）。\n\n"
     "安全与恢复：\n"
     "  修改 /etc/default/grub 前自动备份为 /etc/default/grub.msconfig-bak\n"
     "  重新生成前备份 grub.cfg 为 grub.cfg.msconfig-bak\n"
     "  恢复命令：sudo cp /etc/default/grub.msconfig-bak /etc/default/grub && sudo grub-mkconfig -o /boot/grub/grub.cfg\n"
     "  恢复服务：sudo systemctl enable 服务名",
     "msconfig for Linux - a Linux recreation of Windows msconfig (C++/Qt).\n\n"
     "• General: normal/diagnostic/selective startup (systemd default target + bulk toggles)\n"
     "• Boot: manage GRUB default entry, timeout and kernel parameters (writes /etc/default/grub and regenerates grub.cfg)\n"
     "• Services: enable/disable systemd services (takes effect next boot, like msconfig)\n"
     "• Startup: XDG-standard autostart items (~/.config/autostart and /etc/xdg/autostart)\n"
     "• Tools: quick access to common system tools\n\n"
     "Privilege elevation adapts automatically: run as root -> pkexec -> sudo in a terminal.\n"
     "Works on any distro (Arch/Debian/Fedora/openSUSE...) and any DE/WM (X11/Wayland).\n\n"
     "Safety & recovery:\n"
     "  /etc/default/grub is backed up to /etc/default/grub.msconfig-bak before changes\n"
     "  grub.cfg is backed up to grub.cfg.msconfig-bak before regenerating\n"
     "  Restore: sudo cp /etc/default/grub.msconfig-bak /etc/default/grub && sudo grub-mkconfig -o /boot/grub/grub.cfg\n"
     "  Restore a service: sudo systemctl enable <name>"},
};

bool s_zh = true;
} // namespace

void initLang()
{
    s_zh = (QLocale::system().language() == QLocale::Chinese);
    if (!s_zh) {
        // 兜底：某些环境 QLocale 探测不准，再看 LANG/LC_ALL
        const auto env = QProcessEnvironment::systemEnvironment();
        const QStringList keys = {"LC_ALL", "LC_MESSAGES", "LANG"};
        for (const QString& k : keys) {
            const QString v = env.value(k);
            if (v.startsWith("zh", Qt::CaseInsensitive)) {
                s_zh = true;
                break;
            }
        }
    }
}

QString L(const char* key)
{
    for (const Ent& e : kTable) {
        if (std::strcmp(e.key, key) == 0)
            return QString::fromUtf8(s_zh ? e.zh : e.en);
    }
    return QString::fromUtf8(key);
}

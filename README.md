# msconfig for Linux

Windows「系统配置 (msconfig)」的 Linux 复刻，使用 **C++ / Qt Widgets** 编写，
零第三方依赖，**跨发行版、跨桌面环境/WM（X11 与 Wayland）**可用。

## 功能（与 msconfig 一一对应）

| 标签页 | 功能 | 实现方式 |
|---|---|---|
| 常规 | 正常 / 诊断 / 有选择的启动 | 诊断启动 = `systemctl set-default multi-user.target` + 禁用自启动项；有选择的启动 = 批量开关系统服务与自启动项（记录清单，可一键还原） |
| 引导 | 默认启动项、超时、内核参数 | 改写 `/etc/default/grub` 并重新生成 `grub.cfg`（`update-grub` / `grub2-mkconfig` / `grub-mkconfig` 自动适配） |
| 服务 | 启用/禁用 systemd 服务 | `systemctl enable/disable`（下次启动生效，与 msconfig 语义一致）；支持「隐藏系统自带服务」过滤 |
| 启动 | 自启动项管理 | XDG 标准：`~/.config/autostart` + `/etc/xdg/autostart`，禁用系统级条目时自动生成用户级遮挡文件 |
| 工具 | 常用系统工具入口 | 系统信息 / 进程 / 服务 / 磁盘 / 网络 / 日志 / 软件包 / grub-editenv，终端模拟器自动探测（17 种 + `$TERMINAL`） |

界面为**中英双语**，按系统语言自动切换。

## 构建

唯一依赖：CMake ≥ 3.16、C++17 编译器、Qt5 或 Qt6 的 Widgets 模块（Qt6 优先，自动回退 Qt5）。

```bash
./build.sh          # 或：cmake -B build && cmake --build build
./build/msconfig    # 运行
./build/msconfig --check   # 无 GUI 后端自检（打印检测结果，不修改任何东西）
```

各发行版安装依赖：

```bash
# Arch / CachyOS / Manjaro
sudo pacman -S cmake qt6-base

# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev
# （老版本 Debian/Ubuntu 用 qtbase5-dev 也可）

# Fedora
sudo dnf install cmake gcc-c++ qt6-qtbase-devel

# openSUSE
sudo zypper install cmake gcc-c++ qt6-widgets-devel
```

## 权限（自动适配，任意 DE/WM）

程序平时以普通用户运行，只有真正需要 root 的操作才按以下顺序提权：

1. 以 root 运行本程序时直接执行；
2. 有 `pkexec`（polkit）→ 弹出当前桌面环境的认证框（GNOME/KDE/XFCE 均自带 agent）；
3. 无 polkey 的极简环境 → 自动在探测到的终端里运行 `sudo`。

Qt 程序在 Wayland 下原生运行，在 X11 与 XWayland 下同样正常；不依赖任何特定 DE 的组件。

## 安全与恢复

- 修改 `/etc/default/grub` 前自动备份为 `/etc/default/grub.msconfig-bak`（仅首次）；
- 重新生成前备份 `grub.cfg` 为 `grub.cfg.msconfig-bak`（仅首次）；
- 「常规」页批量禁用的服务/自启动项都记录在 `~/.config/msconfig-linux/state.json`，改回「正常启动」即自动还原。

出问题时手动恢复：

```bash
sudo cp /etc/default/grub.msconfig-bak /etc/default/grub
sudo grub-mkconfig -o /boot/grub/grub.cfg     # Fedora 为 /boot/grub2/grub.cfg
sudo systemctl enable <服务名>                 # 恢复服务
```

> 引导页仅支持 GRUB。若你的系统使用 systemd-boot / Limine / rEFInd，程序会给出提示。

## 目录结构

```
src/
├── main.cpp        入口 + --check/--version
├── lang.cpp/.h     中英双语字符串表
├── util.cpp/.h     提权(pkexec→sudo)、终端探测、发行版/会话识别、状态存取
├── grub.cpp/.h     grub.cfg 菜单解析、/etc/default/grub 保序改写、mkconfig 命令适配
├── services.cpp/.h systemd 服务列表与保护名单
├── autostart.cpp/.h XDG 自启动项（含 .desktop 迷你保序解析器与遮挡机制）
├── mainwindow.cpp/.h  主窗口（五标签 + 确定/取消/应用/帮助）
└── *tab.cpp/.h     各标签页
```

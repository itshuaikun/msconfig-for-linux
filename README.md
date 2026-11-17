# msconfig for Linux — CachyOS adaptation

基于 [macOS-Terminal/msconfig-for-linux](https://github.com/macOS-Terminal/msconfig-for-linux) 的 C++ / Qt Widgets 系统配置工具。此 fork 面向 **CachyOS + KDE Plasma + Wayland + systemd + Limine**，保留 Qt5 / Qt6 构建能力。

## 当前能力与边界

| 页面 | 当前行为 |
|---|---|
| 常规 | **只读**显示旧版保存的模式与恢复记录数量。暂不提供批量启动切换或自动恢复：旧格式没有完整保存原始 target、服务启用类型等信息。 |
| 引导 | 从 EFI Boot Loader Interface 识别实际引导器。**Limine 仅显示版本、本次启动项和内核参数**，不编辑默认项、超时或内核参数。GRUB 保留原有编辑功能及部分安全修复，尚未在此机器验证写入。 |
| 服务 | 管理 systemd 持久启用/禁用；各禁用入口统一保护关键服务；批量操作遵守本地服务筛选。运行时启用项暂为只读，避免被转换为持久启用。 |
| 启动 | 管理 XDG 自启动项，支持 `XDG_CONFIG_HOME`、`XDG_CONFIG_DIRS` 优先级，以及 KDE 的 `OnlyShowIn`、`NotShowIn`、`TryExec` 适用性判断；只写用户级配置。 |
| 工具 | 系统、服务、网络、日志和软件包信息；新增 `bootctl status`、Limine 菜单、Limine/Snapper 快照入口。缺少对应程序时禁用入口。 |

“本地定义的服务”是按 unit 文件所在目录判断，不等同于第三方软件包。服务和启动页的“启用/禁用”按钮立即操作；复选框由“应用/确定”提交，“取消”不会撤销已经执行的按钮操作。

CachyOS 的引导配置由 `limine-entry-tool`、内核更新钩子和 `limine-snapper-sync` 共同管理。本版本不直接改写生成的 Limine 配置，也不把 GRUB 编辑器套用到 Limine。完整设计审查见 [架构与适配审查](docs/architecture-review.md)。

## 构建与验证

依赖：CMake ≥ 3.16、C++17 编译器、Qt Widgets（优先 Qt6，回退 Qt5）。CachyOS / Arch 的安装命令：

```bash
sudo pacman -S --needed base-devel cmake qt6-base qt6-wayland polkit
```

```bash
cmake -S . -B out -DCMAKE_BUILD_TYPE=Release
cmake --build out -j4
ctest --test-dir out --output-on-failure
./out/msconfig --check
./out/msconfig
```

`--check` 为无 GUI 的只读探测，不提权、不修改系统。测试的配置写入使用临时目录；GUI 测试默认使用 Qt offscreen 插件。需要测试实际 Wayland 插件时可运行 `QT_QPA_PLATFORM=wayland ./out/ui_test`。

也可使用 `./build.sh`，产物同样位于 `out`。旧上游曾将构建缓存和二进制提交到仓库；此 fork 已停止跟踪这些产物。

安装为可选步骤：

```bash
cmake --install out --prefix "$HOME/.local"
```

安装二进制和桌面入口，需确保 `~/.local/bin` 在桌面会话的 `PATH` 中。

## 权限与恢复

GUI 应以普通用户运行。系统修改使用同步 `pkexec` 授权；缺少 polkit 时返回错误，不再启动无法追踪结果的异步 sudo 操作。工具页由用户明确启动的只读管理命令可以在终端中使用 sudo。

用户状态与自启动文件使用原子写入。旧恢复记录保留，不自动清除，也不能保证从旧记录恢复出原始系统状态。若曾使用上游批量模式，请先核对实际 target、服务和自启动设置，再逐项恢复。

GRUB 修改仍会创建首次备份；临时文件改为随机安全文件、备份失败会中断操作，写入的 shell 特殊字符保持字面值。但完整的并发检查、事务回滚及 GRUB 语法支持尚未实现，详见审查文档。

## 代码结构

- `src/mainwindow.*`、`src/*tab.*`：五个界面页与操作协调。
- `src/bootloader.*`：实际 EFI 引导器检测，不依赖是否安装 GRUB/Limine 二进制。
- `src/grub.*`、`src/services.*`、`src/autostart.*`：各配置领域逻辑。
- `src/util.*`：命令执行、授权、系统探测与状态持久化。
- `tests/`：核心回归、自启动隔离测试、GUI 冒烟与只读保护测试。

MIT License，保留上游版权。

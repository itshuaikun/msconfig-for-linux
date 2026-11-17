#pragma once
#include <QMainWindow>

class QTabWidget;
class BootTab;
class GeneralTab;
class ServicesTab;
class StartupTab;
class ToolsTab;

// 主窗口：msconfig 式五标签布局 + 底部 确定/取消/应用/帮助
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // 模态显示命令输出（grub-mkconfig / systemctl 结果）
    static void showOutput(QWidget* parent, const QString& title, const QString& text);

private slots:
    bool onApply();
    void onOk();
    void onHelp();

private:
    QTabWidget* m_tabs = nullptr;
    GeneralTab* m_general = nullptr;
    BootTab* m_boot = nullptr;
    ServicesTab* m_services = nullptr;
    StartupTab* m_startup = nullptr;
    ToolsTab* m_tools = nullptr;
};

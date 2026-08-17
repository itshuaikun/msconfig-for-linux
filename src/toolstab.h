#pragma once
#include "configtab.h"

class QListWidget;
class QPlainTextEdit;
class QPushButton;

// 「工具」标签页：常用系统工具入口（对应 msconfig 工具页）
class ToolsTab : public ConfigTab {
    Q_OBJECT
public:
    explicit ToolsTab(QWidget* parent = nullptr);

    QStringList apply() override; // 无待应用项
    void reload() override {}

private slots:
    void onSelectionChanged();
    void onLaunch();

private:
    QListWidget* m_list = nullptr;
    QPlainTextEdit* m_detail = nullptr;
    QPushButton* m_launchBtn = nullptr;
};

#pragma once
#include "autostart.h"
#include "configtab.h"

class QPushButton;
class QTreeWidget;

// 「启动」标签页：XDG 标准自启动项（对应 msconfig 启动页）
class StartupTab : public ConfigTab {
    Q_OBJECT
public:
    explicit StartupTab(QWidget* parent = nullptr);

    QStringList apply() override;
    void reload() override;

private slots:
    void onEnableSelected();
    void onDisableSelected();

private:
    bool applyUnits(const QStringList& fileNames, bool enable);

    QTreeWidget* m_tree = nullptr;
    QPushButton* m_enableBtn = nullptr;
    QPushButton* m_disableBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QVector<AutoInfo> m_rows;
};

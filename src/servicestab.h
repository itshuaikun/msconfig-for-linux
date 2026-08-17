#pragma once
#include "configtab.h"
#include "services.h"

class QCheckBox;
class QPushButton;
class QTreeWidget;

// 「服务」标签页：systemd 服务启用/禁用（对应 msconfig 服务页，下次启动生效）
class ServicesTab : public ConfigTab {
    Q_OBJECT
public:
    explicit ServicesTab(QWidget* parent = nullptr);

    QStringList apply() override;
    void reload() override;

private slots:
    void onEnableSelected();
    void onDisableSelected();
    void onEnableAll();
    void onDisableAll();
    void onHideToggle();

private:
    QStringList applyUnits(const QStringList& units, bool enable);
    void populate();

    QTreeWidget* m_tree = nullptr;
    QCheckBox* m_hideSystem = nullptr;
    QPushButton* m_enableBtn = nullptr;
    QPushButton* m_disableBtn = nullptr;
    QPushButton* m_enableAllBtn = nullptr;
    QPushButton* m_disableAllBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QVector<ServiceInfo> m_rows; // index == 树条目 UserRole
};

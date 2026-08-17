#pragma once
#include <QHash>
#include "configtab.h"
#include "grub.h"

class QCheckBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTreeWidget;

// 「引导」标签页：GRUB 默认启动项 / 超时 / 内核参数（对应 msconfig 引导页）
class BootTab : public ConfigTab {
    Q_OBJECT
public:
    explicit BootTab(QWidget* parent = nullptr);

    QStringList apply() override;
    void reload() override;

private slots:
    void onSetDefault();
    void onAdvanced();
    void onRegen();
    void onItemDoubleClicked(class QTreeWidgetItem* item, int column);

private:
    QString resolveCurrentDefault(const QHash<QString, QString>& defs);
    void populateTree();
    void syncCheckboxesFromCmdline();

    bool m_hasGrub = false;
    QVector<GrubEntry> m_entries;
    QString m_currentDefault; // 当前 grub.cfg/默认文件中的默认条目路径
    QString m_chosenDefault;  // 用户选择的新默认（空 = 不更改）
    QString m_cmdlineDefault; // GRUB_CMDLINE_LINUX_DEFAULT 原始串
    QString m_cmdlineAll;     // GRUB_CMDLINE_LINUX 原始串

    QLabel* m_noGrubLabel = nullptr;
    QLabel* m_defaultLabel = nullptr;
    QTreeWidget* m_tree = nullptr;
    QCheckBox* m_quiet = nullptr;
    QCheckBox* m_splash = nullptr;
    QCheckBox* m_safeBoot = nullptr;
    QSpinBox* m_timeout = nullptr;
    QPushButton* m_setDefaultBtn = nullptr;
    QPushButton* m_advancedBtn = nullptr;
    QPushButton* m_regenBtn = nullptr;
};

#pragma once
#include "configtab.h"

class QCheckBox;
class QLabel;
class QRadioButton;

// 「常规」标签页：正常/诊断/有选择的启动（对应 msconfig 常规页）
// - 诊断启动   = systemd 默认目标切到 multi-user.target + 禁用全部自启动项
// - 有选择的启动 = 按勾选批量控制系统服务 / 自启动项（记录清单，可一键还原）
class GeneralTab : public ConfigTab {
    Q_OBJECT
public:
    explicit GeneralTab(QWidget* parent = nullptr);

    QStringList apply() override;
    void reload() override;

private:
    void setControlsEnabled(bool on);

    QRadioButton* m_normal = nullptr;
    QRadioButton* m_diag = nullptr;
    QRadioButton* m_sel = nullptr;
    QCheckBox* m_loadSvcs = nullptr;
    QCheckBox* m_loadStartup = nullptr;
    QLabel* m_note = nullptr;

    // 载入时的持久状态
    QString m_mode = "normal";
    bool m_loadedSvc = true;
    bool m_loadedStartup = true;
};

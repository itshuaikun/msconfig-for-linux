#pragma once
#include "configtab.h"

class QCheckBox;
class QLabel;
class QRadioButton;

// Read-only view of legacy startup selections. Bulk changes remain disabled
// until the recovery format can preserve and restore exact prior system state.
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
};

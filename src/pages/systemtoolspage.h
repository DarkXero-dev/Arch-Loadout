#pragma once
#include <QWizardPage>
#include <QCheckBox>

class MainWizard;

class SystemToolsPage : public QWizardPage {
    Q_OBJECT
public:
    explicit SystemToolsPage(MainWizard *wizard);
    void initializePage() override;
    bool validatePage() override;

private:
    MainWizard *m_wiz;

    // Package Managers
    QCheckBox *m_cbOctopi   = nullptr;
    QCheckBox *m_cbPacseek  = nullptr;
    QCheckBox *m_cbBazaar   = nullptr;

    // Driver Extras
    QCheckBox *m_cbRocm     = nullptr;
    QCheckBox *m_cbCuda     = nullptr;
    QCheckBox *m_cbRog      = nullptr;
};

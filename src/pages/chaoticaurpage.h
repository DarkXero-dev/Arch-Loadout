#pragma once
#include <QWizardPage>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QWidget>

class MainWizard;

class ChaoticAURPage : public QWizardPage {
    Q_OBJECT
public:
    explicit ChaoticAURPage(MainWizard *wizard);
    void initializePage() override;
    bool validatePage() override;

private slots:
    void onRepoChoiceChanged();

private:
    MainWizard   *m_wiz;

    // Detected state (set in initializePage)
    bool          m_chaoticActivated = false;   // [chaotic-aur] already in pacman.conf
    QString       m_detectedHelper;             // "paru", "yay", or ""

    // Status bar widgets
    QLabel       *m_chaoticStatusDot  = nullptr;
    QLabel       *m_chaoticStatusText = nullptr;
    QLabel       *m_helperStatusDot   = nullptr;
    QLabel       *m_helperStatusText  = nullptr;

    // Repo setup (disabled when already activated)
    QGroupBox    *m_setupGroup    = nullptr;
    QRadioButton *m_radioEnable   = nullptr;
    QRadioButton *m_radioSkip     = nullptr;
    QLabel       *m_warnLabel     = nullptr;

    // AUR helper (always visible)
    QGroupBox    *m_aurHelperGroup  = nullptr;
    QWidget      *m_aurChooseWidget = nullptr;  // radio row, shown when no helper detected
    QRadioButton *m_radioParu       = nullptr;
    QRadioButton *m_radioYay        = nullptr;

    // System update
    QCheckBox    *m_cbSysUpgrade  = nullptr;

    // CachyOS Kernel (requires chaotic-aur to be active/enabled)
    QGroupBox    *m_kernelGroup   = nullptr;
    QCheckBox    *m_cbCachyKernel = nullptr;
};

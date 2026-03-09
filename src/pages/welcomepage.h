#pragma once
#include <QWizardPage>

class MainWizard;

class WelcomePage : public QWizardPage {
    Q_OBJECT
public:
    explicit WelcomePage(MainWizard *wizard);
    void initializePage() override;
    bool isComplete() const override;
private:
    MainWizard *m_wiz;
};

#pragma once
#include <QWizardPage>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QLabel>
#include <QThread>
#include "../installworker.h"

class MainWizard;

class InstallPage : public QWizardPage {
    Q_OBJECT
public:
    explicit InstallPage(MainWizard *wizard);
    ~InstallPage();
    void initializePage() override;
    bool isComplete() const override { return m_done; }

private slots:
    void onStepStarted(const QString &id, const QString &desc);
    void onStepFinished(const QString &id, bool ok, int code);
    void onStepSkipped(const QString &id, const QString &desc);
    void onLogLine(const QString &line);
    void onAllDone(int errorCount);

private:
    void startInstall();

    MainWizard     *m_wiz;
    QLabel         *m_currentStep  = nullptr;
    QProgressBar   *m_progress     = nullptr;
    QPlainTextEdit *m_log          = nullptr;
    QLabel         *m_resultLabel  = nullptr;
    QThread        *m_thread       = nullptr;
    InstallWorker  *m_worker       = nullptr;
    bool            m_done         = false;
    int             m_totalSteps   = 0;
    int             m_stepsDone    = 0;
};

#include "installpage.h"
#include "../mainwizard.h"
#include "../pagehelpers.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollBar>

InstallPage::InstallPage(MainWizard *wizard) : QWizardPage(wizard), m_wiz(wizard)
{
}

InstallPage::~InstallPage()
{
    if (m_worker) m_worker->cancel();
    if (m_thread) { m_thread->quit(); m_thread->wait(3000); }
}

void InstallPage::initializePage()
{
    // Disable back/cancel while installing
    wizard()->button(QWizard::BackButton)->setEnabled(false);
    wizard()->button(QWizard::CancelButton)->setEnabled(false);

    if (layout()) {
        QLayoutItem *i;
        while ((i = layout()->takeAt(0))) {
            if (i->widget()) i->widget()->deleteLater();
            delete i;
        }
        delete layout();
    }
    m_done      = false;
    m_totalSteps = 0;
    m_stepsDone  = 0;

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(8);

    outer->addWidget(makePageHeader(
        "\xF0\x9F\x9B\xA0",   // 🛠
        "Installing",
        "Applying your selections — please wait"
    ));

    // Current step label
    m_currentStep = new QLabel("Preparing\u2026");
    m_currentStep->setWordWrap(true);
    outer->addWidget(m_currentStep);

    // Progress bar
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    outer->addWidget(m_progress);

    // Log output
    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    m_log->setFont(QFont("Monospace", 9));
    m_log->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    outer->addWidget(m_log, 1);

    // Result label (hidden until done)
    m_resultLabel = new QLabel;
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setTextFormat(Qt::RichText);
    m_resultLabel->setVisible(false);
    outer->addWidget(m_resultLabel);

    startInstall();
}

void InstallPage::startInstall()
{
    const QList<InstallStep> steps = m_wiz->buildSteps();

    if (steps.isEmpty()) {
        m_currentStep->setText("Nothing to do — no packages were selected.");
        m_progress->setValue(100);
        m_resultLabel->setText(
            "<span style='color:#22c55e;'><b>\u2714 Nothing to install.</b></span> "
            "Click <b>Finish</b> to exit.");
        m_resultLabel->setVisible(true);
        m_done = true;
        emit completeChanged();
        wizard()->button(QWizard::FinishButton)->setEnabled(true);
        return;
    }

    m_totalSteps = steps.size();
    m_stepsDone  = 0;
    m_progress->setRange(0, m_totalSteps);
    m_progress->setValue(0);

    m_thread = new QThread(this);
    m_worker = new InstallWorker;
    m_worker->setSteps(steps);
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started,          m_worker, &InstallWorker::run);
    connect(m_worker, &InstallWorker::stepStarted, this,    &InstallPage::onStepStarted);
    connect(m_worker, &InstallWorker::stepFinished,this,    &InstallPage::onStepFinished);
    connect(m_worker, &InstallWorker::stepSkipped, this,    &InstallPage::onStepSkipped);
    connect(m_worker, &InstallWorker::logLine,     this,    &InstallPage::onLogLine);
    connect(m_worker, &InstallWorker::allDone,     this,    &InstallPage::onAllDone);
    connect(m_worker, &InstallWorker::allDone,     m_thread,&QThread::quit);
    connect(m_thread, &QThread::finished,          m_worker,&QObject::deleteLater);

    m_thread->start();
}

void InstallPage::onStepStarted(const QString & /*id*/, const QString &desc)
{
    m_currentStep->setText(QString("\u23F3 %1").arg(desc));
    onLogLine(QString("\n==> %1").arg(desc));
}

void InstallPage::onStepFinished(const QString & /*id*/, bool ok, int /*code*/)
{
    ++m_stepsDone;
    m_progress->setValue(m_stepsDone);
    if (!ok)
        onLogLine("  [FAILED]");
}

void InstallPage::onStepSkipped(const QString & /*id*/, const QString &desc)
{
    ++m_stepsDone;
    m_progress->setValue(m_stepsDone);
    onLogLine(QString("  (already done: %1)").arg(desc));
}

void InstallPage::onLogLine(const QString &line)
{
    m_log->appendPlainText(line);
    // Auto-scroll to bottom
    QScrollBar *sb = m_log->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void InstallPage::onAllDone(int errorCount)
{
    m_worker = nullptr;   // will be deleteLater'd by thread finished signal

    if (errorCount == 0) {
        m_currentStep->setText("\u2714 Installation complete.");
        m_resultLabel->setText(
            "<span style='color:#22c55e;'><b>\u2714 All steps completed successfully.</b></span><br>"
            "Click <b>Finish</b> to close.");
        m_resultLabel->setStyleSheet(
            "QLabel { background: #14532d; color: #dcfce7; border-radius: 6px; padding: 8px; }");
    } else {
        m_currentStep->setText(QString("\u26A0 Finished with %1 error(s).").arg(errorCount));
        m_resultLabel->setText(
            QString("<b>\u26A0 %1 step(s) failed.</b><br>"
                    "Review the log above for details. "
                    "You can still click <b>Finish</b> to exit.").arg(errorCount));
        m_resultLabel->setStyleSheet(
            "QLabel { background: #7a0000; color: #fee; border-radius: 6px; padding: 8px; }");
    }

    m_resultLabel->setVisible(true);
    m_done = true;
    emit completeChanged();
    wizard()->button(QWizard::FinishButton)->setEnabled(true);
    wizard()->button(QWizard::CancelButton)->setEnabled(true);
}

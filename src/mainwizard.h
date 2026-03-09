#pragma once
#include <QWizard>
#include <QMap>
#include <QVariant>
#include "installworker.h"

enum PageId {
    PAGE_WELCOME = 0,
    PAGE_CHAOTICAUR,
    PAGE_SYSTEMTOOLS,
    PAGE_INSTALL,
};

class MainWizard : public QWizard {
    Q_OBJECT
public:
    explicit MainWizard(QWidget *parent = nullptr);

    void     setOpt(const QString &key, const QVariant &val);
    QVariant getOpt(const QString &key, const QVariant &def = {}) const;

    void accept() override;   // Finish → restart wizard instead of closing

    QString targetUser() const { return m_targetUser; }

    QList<InstallStep> buildSteps() const;

private:
    void detectSystem();
    QString                 m_targetUser;
    QMap<QString,QVariant>  m_opts;
};

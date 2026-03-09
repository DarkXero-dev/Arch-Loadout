#include "welcomepage.h"
#include "../mainwizard.h"
#include "../pagehelpers.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFile>
#include <QDir>
#include <unistd.h>

WelcomePage::WelcomePage(MainWizard *wizard) : QWizardPage(wizard), m_wiz(wizard)
{
}

void WelcomePage::initializePage()
{
    if (layout()) {
        QLayoutItem *item;
        while ((item = layout()->takeAt(0))) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete layout();
    }

    auto *lay = new QVBoxLayout(this);
    lay->setSpacing(12);
    lay->setContentsMargins(0, 0, 0, 0);

    // Arch Linux logo header
    lay->addWidget(makePageHeader(
        makeArchLogoPixmap(36),
        "Arch System Loadout",
        "Post-install setup wizard for Arch Linux"
    ));

    // Root warning
    if (geteuid() != 0) {
        auto *warnFrame = new QFrame;
        warnFrame->setFrameShape(QFrame::StyledPanel);
        warnFrame->setStyleSheet("QFrame { background: #7a0000; border-radius: 6px; padding: 4px; }");
        auto *wl = new QVBoxLayout(warnFrame);
        auto *warnLabel = new QLabel(
            "<b style='color:#fee;'>Warning: This application must be run as root.</b><br>"
            "<span style='color:#fee;'>Please restart with: <tt>pkexec arch-system-loadout</tt></span>"
        );
        warnLabel->setWordWrap(true);
        wl->addWidget(warnLabel);
        lay->addWidget(warnFrame);
    }

    // System info — pure file reads, zero QProcess, zero threads
    auto *infoFrame = new QFrame;
    infoFrame->setFrameShape(QFrame::StyledPanel);
    auto *fl = new QVBoxLayout(infoFrame);
    fl->addWidget(new QLabel("<h3>System Information</h3>"));

    QString kernelVer = "Unknown";
    {
        QFile f("/proc/sys/kernel/osrelease");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            kernelVer = QString::fromUtf8(f.readAll()).trimmed();
    }

    QString osName = "Arch Linux";
    {
        QFile f("/etc/os-release");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            for (const QString &line : QString::fromUtf8(f.readAll()).split('\n')) {
                if (line.startsWith("PRETTY_NAME=")) {
                    osName = line.mid(12).remove('"').trimmed();
                    break;
                }
            }
        }
    }

    // KDE Plasma version: read pacman local DB — no process spawn
    QString plasmaVer = "Not detected";
    {
        const QStringList entries = QDir("/var/lib/pacman/local")
                                        .entryList({"plasma-desktop-*"}, QDir::Dirs);
        if (!entries.isEmpty()) {
            QFile desc("/var/lib/pacman/local/" + entries.first() + "/desc");
            if (desc.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QStringList lines = QString::fromUtf8(desc.readAll()).split('\n');
                for (int i = 0; i + 1 < lines.size(); ++i) {
                    if (lines[i].trimmed() == "%VERSION%") {
                        plasmaVer = lines[i + 1].trimmed();
                        break;
                    }
                }
            }
        }
    }

    fl->addWidget(new QLabel(QString("<b>OS:</b> %1").arg(osName.toHtmlEscaped())));
    fl->addWidget(new QLabel(QString("<b>Kernel:</b> %1").arg(kernelVer.toHtmlEscaped())));
    fl->addWidget(new QLabel(QString("<b>KDE Plasma:</b> %1").arg(plasmaVer.toHtmlEscaped())));
    fl->addWidget(new QLabel(
        QString("<b>Target User:</b> %1  "
                "<span style='color:gray;'>(user-level tools installed for this account)</span>")
        .arg(m_wiz->targetUser().toHtmlEscaped())
    ));
    lay->addWidget(infoFrame);

    // Description
    auto *desc = new QLabel(
        "<p>This wizard will let you choose exactly what to install on your Arch Linux system. "
        "Nothing is selected by default — every choice is yours.</p>"
        "<p>You can go back and change selections at any time before the install begins. "
        "Once installation starts it will run to completion.</p>"
        "<p><b>A working internet connection is required.</b></p>"
    );
    desc->setWordWrap(true);
    lay->addWidget(desc);

    // Checkbox explanation
    auto *checkFrame = new QFrame;
    checkFrame->setFrameShape(QFrame::StyledPanel);
    auto *cl = new QVBoxLayout(checkFrame);
    auto *checkLabel = new QLabel(
        "<b>How the checkboxes work:</b><br>"
        "Each item shows its current installed state with a coloured badge.<br>"
        "<span style='color:#3db03d;'><b>[Installed]</b></span> — "
        "already on your system. Ticking reinstalls / ensures it is up to date.<br>"
        "<span style='color:#cc7700;'><b>[Not Installed]</b></span> — "
        "ticking installs it.<br>"
        "Items left unticked are not changed."
    );
    checkLabel->setWordWrap(true);
    cl->addWidget(checkLabel);
    lay->addWidget(checkFrame);

    // Purple circuit decoration fills all remaining space
    lay->addWidget(new CircuitWidget, 1);
}

bool WelcomePage::isComplete() const
{
    return geteuid() == 0;
}

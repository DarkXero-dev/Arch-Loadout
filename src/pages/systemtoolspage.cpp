#include "systemtoolspage.h"
#include "../mainwizard.h"
#include "../pagehelpers.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFrame>

SystemToolsPage::SystemToolsPage(MainWizard *wizard) : QWizardPage(wizard), m_wiz(wizard)
{
}

void SystemToolsPage::initializePage()
{
    if (layout()) {
        QLayoutItem *i;
        while ((i = layout()->takeAt(0))) {
            if (i->widget()) i->widget()->deleteLater();
            delete i;
        }
        delete layout();
    }

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(8);

    outer->addWidget(makePageHeader(
        "\xF0\x9F\x94\xA7",   // 🔧
        "System Tools",
        "Package managers and hardware driver extras"
    ));

    // ── Package Managers ──────────────────────────────────────────────
    auto *pkgGroup = new QGroupBox("Package Managers");
    auto *pkgLayout = new QVBoxLayout(pkgGroup);
    pkgLayout->setSpacing(4);
    pkgLayout->setContentsMargins(10, 6, 10, 6);

    m_cbOctopi  = makeItemRow(pkgGroup, pkgLayout,
        "Octopi  \u2014  Qt-based graphical package manager",
        isPacmanInstalled("octopi"));

    m_cbPacseek = makeItemRow(pkgGroup, pkgLayout,
        "Pacseek  \u2014  terminal UI package browser / search",
        isPacmanInstalled("pacseek"));

    m_cbBazaar  = makeItemRow(pkgGroup, pkgLayout,
        "Bazaar  \u2014  minimal graphical package manager",
        isPacmanInstalled("bazaar"));

    outer->addWidget(pkgGroup);

    // ── Driver Extras ─────────────────────────────────────────────────
    auto *drvGroup = new QGroupBox("Driver Extras");
    auto *drvLayout = new QVBoxLayout(drvGroup);
    drvLayout->setSpacing(4);
    drvLayout->setContentsMargins(10, 6, 10, 6);

    m_cbRocm = makeItemRow(drvGroup, drvLayout,
        "AMD ROCm  \u2014  GPU compute stack  (rocm-hip-sdk  rocm-opencl-sdk)",
        isPacmanInstalledAny({"rocm-hip-sdk", "rocm-opencl-sdk"}));

    m_cbCuda = makeItemRow(drvGroup, drvLayout,
        "nVidia CUDA  \u2014  GPU compute toolkit  (cuda)",
        isPacmanInstalled("cuda"));

    m_cbRog = makeItemRow(drvGroup, drvLayout,
        "ASUS ROG Tools  \u2014  rog-control-center  asusctl  supergfxctl  (+ enables asusd / supergfxd)",
        isPacmanInstalledAny({"rog-control-center", "asusctl", "supergfxctl"}));

    outer->addWidget(drvGroup);
    outer->addStretch(1);
}

bool SystemToolsPage::validatePage()
{
    m_wiz->setOpt("systemtools/octopi",  m_cbOctopi ->isChecked());
    m_wiz->setOpt("systemtools/pacseek", m_cbPacseek->isChecked());
    m_wiz->setOpt("systemtools/bazaar",  m_cbBazaar ->isChecked());
    m_wiz->setOpt("systemtools/rocm",    m_cbRocm   ->isChecked());
    m_wiz->setOpt("systemtools/cuda",    m_cbCuda   ->isChecked());
    m_wiz->setOpt("systemtools/rog",     m_cbRog    ->isChecked());
    return true;
}

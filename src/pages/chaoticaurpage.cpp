#include "chaoticaurpage.h"
#include "../mainwizard.h"
#include "../pagehelpers.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QFile>

// ── Detection helpers ─────────────────────────────────────────────────────────

static bool detectChaoticAUR()
{
    QFile f("/etc/pacman.conf");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    return QString::fromUtf8(f.readAll()).contains("[chaotic-aur]");
}

static QString detectAurHelper()
{
    static const QStringList searchDirs = {"/usr/bin", "/usr/local/bin", "/bin"};
    for (const QString &h : {"paru", "yay"})
        for (const QString &dir : searchDirs)
            if (QFile::exists(dir + "/" + h))
                return h;
    return {};
}

// ── Status indicator helpers ──────────────────────────────────────────────────

static QLabel *makeDot(bool active)
{
    auto *l = new QLabel(active ? "\u25CF" : "\u25CB");   // ● or ○
    l->setStyleSheet(active
        ? "QLabel { color: #22c55e; font-size: 16px; }"
        : "QLabel { color: #6b7280; font-size: 16px; }");
    l->setFixedWidth(18);
    return l;
}

// ── Page ──────────────────────────────────────────────────────────────────────

ChaoticAURPage::ChaoticAURPage(MainWizard *wizard) : QWizardPage(wizard), m_wiz(wizard)
{
}

void ChaoticAURPage::initializePage()
{
    if (layout()) {
        QLayoutItem *i;
        while ((i = layout()->takeAt(0))) {
            if (i->widget()) i->widget()->deleteLater();
            delete i;
        }
        delete layout();
    }

    m_chaoticActivated = detectChaoticAUR();
    m_detectedHelper   = detectAurHelper();

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(6);

    // ── Header ────────────────────────────────────────────────────────
    outer->addWidget(makePageHeader(
        "\xE2\x9A\xA1",
        "Chaotic Repo / AUR Helper",
        "Pre-built AUR packages \u00B7 AUR helper setup \u00B7 kernel \u00B7 schedulers"
    ));

    // ── Status bar ────────────────────────────────────────────────────
    auto *statusFrame = new QFrame;
    statusFrame->setFrameShape(QFrame::StyledPanel);
    auto *statusRow = new QHBoxLayout(statusFrame);
    statusRow->setContentsMargins(12, 6, 12, 6);
    statusRow->setSpacing(6);

    m_chaoticStatusDot  = makeDot(m_chaoticActivated);
    m_chaoticStatusText = new QLabel(
        m_chaoticActivated
            ? "<b>Chaotic-AUR</b> &nbsp;Activated"
            : "<b>Chaotic-AUR</b> &nbsp;Not configured");
    m_chaoticStatusText->setTextFormat(Qt::RichText);

    statusRow->addWidget(m_chaoticStatusDot);
    statusRow->addWidget(m_chaoticStatusText);
    statusRow->addSpacing(20);

    auto *divider = new QFrame;
    divider->setFrameShape(QFrame::VLine);
    divider->setFrameShadow(QFrame::Sunken);
    statusRow->addWidget(divider);
    statusRow->addSpacing(20);

    bool helperOk = !m_detectedHelper.isEmpty();
    m_helperStatusDot  = makeDot(helperOk);
    m_helperStatusText = new QLabel(
        helperOk
            ? QString("<b>AUR Helper</b> &nbsp;%1 detected").arg(m_detectedHelper)
            : "<b>AUR Helper</b> &nbsp;None detected");
    m_helperStatusText->setTextFormat(Qt::RichText);

    statusRow->addWidget(m_helperStatusDot);
    statusRow->addWidget(m_helperStatusText);
    statusRow->addStretch();
    outer->addWidget(statusFrame);

    // ── What is Chaotic-AUR? ──────────────────────────────────────────
    auto *infoFrame = new QFrame;
    infoFrame->setFrameShape(QFrame::StyledPanel);
    auto *infoLayout = new QVBoxLayout(infoFrame);
    infoLayout->setContentsMargins(10, 6, 10, 6);
    auto *infoLabel = new QLabel(
        "<b>What is Chaotic-AUR?</b><br>"
        "A repository of pre-compiled AUR packages — install popular AUR software directly "
        "with <tt>pacman</tt>, no compilation needed. Without it, AUR packages must be "
        "built from source (can take <b>30+ minutes</b> for packages like the CachyOS kernel, "
        "Brave, or Google Chrome)."
    );
    infoLabel->setWordWrap(true);
    infoLayout->addWidget(infoLabel);
    outer->addWidget(infoFrame);

    // ── Repository Setup ──────────────────────────────────────────────
    m_setupGroup = new QGroupBox("Repository Setup");
    auto *setupLayout = new QVBoxLayout(m_setupGroup);
    setupLayout->setSpacing(4);
    setupLayout->setContentsMargins(10, 6, 10, 6);

    if (m_chaoticActivated) {
        auto *alreadyRow = new QHBoxLayout;
        alreadyRow->addWidget(makeDot(true));
        auto *alreadyLabel = new QLabel(
            "Chaotic-AUR is already configured on this system — no action needed.");
        alreadyLabel->setWordWrap(true);
        alreadyRow->addWidget(alreadyLabel, 1);
        setupLayout->addLayout(alreadyRow);
        m_setupGroup->setEnabled(false);
        m_radioEnable = nullptr;
        m_radioSkip   = nullptr;
        m_warnLabel   = nullptr;
    } else {
        m_radioEnable = new QRadioButton("Enable Chaotic-AUR  (Recommended — fast pacman installs)");
        m_radioEnable->setChecked(true);
        setupLayout->addWidget(m_radioEnable);

        m_radioSkip = new QRadioButton("Skip Chaotic-AUR  (AUR packages compiled from source)");
        setupLayout->addWidget(m_radioSkip);

        connect(m_radioEnable, &QRadioButton::toggled, this, &ChaoticAURPage::onRepoChoiceChanged);
        connect(m_radioSkip,   &QRadioButton::toggled, this, &ChaoticAURPage::onRepoChoiceChanged);
    }
    outer->addWidget(m_setupGroup);

    // Warning — visible only when skipping
    m_warnLabel = new QLabel(
        "\u26A0\uFE0F  <b>Heads up:</b> Without Chaotic-AUR, packages like the CachyOS kernel, "
        "google-chrome, brave, vesktop, vkbasalt, and goverlay must be compiled from AUR source. "
        "Compilation time depends on your hardware and can be substantial."
    );
    m_warnLabel->setWordWrap(true);
    m_warnLabel->setTextFormat(Qt::RichText);
    m_warnLabel->setStyleSheet(
        "QLabel { background: #78420a; color: #fef3c7; border-radius: 6px; padding: 8px; }");
    m_warnLabel->setVisible(false);
    outer->addWidget(m_warnLabel);

    // ── AUR Helper ────────────────────────────────────────────────────
    m_aurHelperGroup = new QGroupBox("AUR Helper");
    auto *aurLayout = new QVBoxLayout(m_aurHelperGroup);
    aurLayout->setSpacing(4);
    aurLayout->setContentsMargins(10, 6, 10, 6);

    if (!m_detectedHelper.isEmpty()) {
        // Helper already present — grey out the whole group
        auto *helperRow = new QHBoxLayout;
        helperRow->addWidget(makeDot(true));
        auto *hl = new QLabel(
            QString("<b>%1</b> is already installed and will be used for AUR packages.")
            .arg(m_detectedHelper));
        hl->setWordWrap(true);
        hl->setTextFormat(Qt::RichText);
        helperRow->addWidget(hl, 1);
        aurLayout->addLayout(helperRow);
        m_aurHelperGroup->setEnabled(false);   // grey out — already configured
        m_aurChooseWidget = nullptr;
        m_radioParu = nullptr;
        m_radioYay  = nullptr;
    } else {
        auto *noneRow = new QHBoxLayout;
        noneRow->addWidget(makeDot(false));
        auto *noneLabel = new QLabel("No AUR helper detected.  Choose one to install:");
        noneRow->addWidget(noneLabel, 1);
        aurLayout->addLayout(noneRow);

        m_aurChooseWidget = new QWidget;
        auto *chooseLayout = new QVBoxLayout(m_aurChooseWidget);
        chooseLayout->setContentsMargins(8, 2, 0, 2);
        chooseLayout->setSpacing(3);

        m_radioParu = new QRadioButton("Paru  (Recommended — feature-rich, written in Rust)");
        m_radioParu->setChecked(true);
        chooseLayout->addWidget(m_radioParu);

        m_radioYay = new QRadioButton("Yay  (popular, written in Go)");
        chooseLayout->addWidget(m_radioYay);

        aurLayout->addWidget(m_aurChooseWidget);
    }
    outer->addWidget(m_aurHelperGroup);

    // ── System upgrade ────────────────────────────────────────────────
    auto *upgradeBox = new QGroupBox("System Update");
    auto *upgradeLayout = new QVBoxLayout(upgradeBox);
    upgradeLayout->setContentsMargins(10, 6, 10, 6);
    m_cbSysUpgrade = new QCheckBox("Run full system upgrade (pacman -Syu) before installing");
    m_cbSysUpgrade->setChecked(true);
    upgradeLayout->addWidget(m_cbSysUpgrade);
    outer->addWidget(upgradeBox);

    // ── CachyOS Kernel ────────────────────────────────────────────────
    m_kernelGroup = new QGroupBox("CachyOS Kernel  (requires Chaotic-AUR)");
    auto *kernelLayout = new QVBoxLayout(m_kernelGroup);
    kernelLayout->setSpacing(4);
    kernelLayout->setContentsMargins(10, 6, 10, 6);

    auto *kernelInfo = new QLabel(
        "Performance-optimised kernel: BORE scheduler, BBRv3 TCP, FUTEX2, upstream patches. "
        "Installed alongside your existing kernel — selectable at boot via GRUB."
    );
    kernelInfo->setWordWrap(true);
    kernelLayout->addWidget(kernelInfo);

    m_cbCachyKernel = new QCheckBox("Install CachyOS Kernel  (linux-cachyos + headers)");
    kernelLayout->addWidget(m_cbCachyKernel);

    outer->addWidget(m_kernelGroup);
    outer->addStretch(1);

    onRepoChoiceChanged();
}

void ChaoticAURPage::onRepoChoiceChanged()
{
    const bool chaoticOn = m_chaoticActivated
                           || (m_radioEnable && m_radioEnable->isChecked());

    if (m_warnLabel)
        m_warnLabel->setVisible(!chaoticOn);

    m_kernelGroup->setEnabled(chaoticOn);

    if (!chaoticOn)
        m_cbCachyKernel->setChecked(false);
}

bool ChaoticAURPage::validatePage()
{
    const bool chaoticOn = m_chaoticActivated
                           || (m_radioEnable && m_radioEnable->isChecked());

    m_wiz->setOpt("setup/chaotic-aur",            chaoticOn);
    m_wiz->setOpt("setup/chaotic-aur-preexisting", m_chaoticActivated);
    m_wiz->setOpt("setup/aur-helper-preexisting",  !m_detectedHelper.isEmpty());
    m_wiz->setOpt("setup/sysupgrade",              m_cbSysUpgrade->isChecked());
    m_wiz->setOpt("setup/cachyos-kernel",          m_cbCachyKernel->isChecked());

    if (!m_detectedHelper.isEmpty()) {
        m_wiz->setOpt("setup/aur-helper", m_detectedHelper);
    } else if (m_radioParu) {
        m_wiz->setOpt("setup/aur-helper", m_radioParu->isChecked() ? "paru" : "yay");
    } else {
        m_wiz->setOpt("setup/aur-helper", QString("yay"));
    }

    return true;
}

#include "mainwizard.h"
#include "pages/welcomepage.h"
#include "pages/chaoticaurpage.h"
#include "pages/systemtoolspage.h"
#include "pages/installpage.h"
#include <pwd.h>
#include <cstdlib>
#include <cstring>
#include <QDir>
#include <QFile>

MainWizard::MainWizard(QWidget *parent) : QWizard(parent)
{
    detectSystem();

    setWindowTitle("Arch System Loadout");
    setMinimumSize(960, 700);
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::NoBackButtonOnStartPage, true);
    setOption(QWizard::NoBackButtonOnLastPage,  true);
    setOption(QWizard::DisabledBackButtonOnLastPage, true);

    setPage(PAGE_WELCOME,      new WelcomePage(this));
    setPage(PAGE_CHAOTICAUR,   new ChaoticAURPage(this));
    setPage(PAGE_SYSTEMTOOLS,  new SystemToolsPage(this));
    setPage(PAGE_INSTALL,      new InstallPage(this));

    setStartId(PAGE_WELCOME);
}

void MainWizard::detectSystem()
{
    // Detect the real (non-root) user who launched the application.
    // Priority: PKEXEC_UID > SUDO_USER
    const QByteArray pkexecUidEnv = qgetenv("PKEXEC_UID");
    const QByteArray sudoUserEnv  = qgetenv("SUDO_USER");

    if (!pkexecUidEnv.isEmpty()) {
        struct passwd *pw = getpwuid(static_cast<uid_t>(pkexecUidEnv.toInt()));
        if (pw && pw->pw_name && std::strcmp(pw->pw_name, "root") != 0)
            m_targetUser = QString::fromUtf8(pw->pw_name);
    }

    if (m_targetUser.isEmpty() && !sudoUserEnv.isEmpty()
            && sudoUserEnv != "root") {
        m_targetUser = QString::fromUtf8(sudoUserEnv);
    }

    // Fallback: first non-root user in /home
    if (m_targetUser.isEmpty()) {
        const QStringList users = QDir("/home").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (!users.isEmpty())
            m_targetUser = users.first();
    }

    if (m_targetUser.isEmpty())
        m_targetUser = "root";
}

void MainWizard::setOpt(const QString &k, const QVariant &v) { m_opts[k] = v; }
QVariant MainWizard::getOpt(const QString &k, const QVariant &def) const { return m_opts.value(k, def); }

void MainWizard::accept()
{
    m_opts.clear();
    restart();   // back to Welcome page instead of closing
}

QList<InstallStep> MainWizard::buildSteps() const
{
    QList<InstallStep> S;
    const QString tu = m_targetUser;
    auto get = [&](const QString &k) { return m_opts.value(k, false).toBool(); };
    const bool chaoticAur          = get("setup/chaotic-aur");
    const bool chaoticPreexisting  = get("setup/chaotic-aur-preexisting");
    const bool helperPreexisting   = get("setup/aur-helper-preexisting");
    const QString aurHelper = m_opts.value("setup/aur-helper", "yay").toString();

    auto pacStep = [&](const QString &id, const QString &desc, const QStringList &pkgs) -> InstallStep {
        QStringList cmd = {"pacman", "-S", "--noconfirm", "--needed"};
        cmd << pkgs;
        return InstallStep{id, desc, cmd};
    };

    // ---- Chaotic-AUR setup (only if wanted AND not already configured) ----
    if (chaoticAur && !chaoticPreexisting) {
        S << InstallStep{"chaotic_key", "Import Chaotic-AUR signing key",
            {"bash", "-c",
             "pacman-key --recv-key 3056513887B78AEB --keyserver keyserver.ubuntu.com && "
             "pacman-key --lsign-key 3056513887B78AEB"}};
        S << InstallStep{"chaotic_keyring", "Install Chaotic-AUR keyring",
            {"pacman", "-U", "--noconfirm",
             "https://cdn-mirror.chaotic.cx/chaotic-aur/chaotic-keyring.pkg.tar.zst"}};
        S << InstallStep{"chaotic_mirrorlist", "Install Chaotic-AUR mirrorlist",
            {"pacman", "-U", "--noconfirm",
             "https://cdn-mirror.chaotic.cx/chaotic-aur/chaotic-mirrorlist.pkg.tar.zst"}};
        S << InstallStep{"chaotic_conf", "Add Chaotic-AUR to pacman.conf",
            {"bash", "-c",
             "grep -q '\\[chaotic-aur\\]' /etc/pacman.conf || "
             "printf '\\n[chaotic-aur]\\nInclude = /etc/pacman.d/chaotic-mirrorlist\\n' >> /etc/pacman.conf"}};
        S << InstallStep{"pacman_sy", "Synchronise pacman databases",
            {"pacman", "-Sy", "--noconfirm"}};
    }

    // ---- AUR helper install (only if no chaotic-aur AND not already present) ----
    if (!chaoticAur && !helperPreexisting) {
        const QString helperBin = aurHelper + "-bin";
        S << InstallStep{aurHelper + "_install",
            QString("Install %1 (AUR helper)").arg(aurHelper),
            {"bash", "-c",
             QString("sudo -u %1 bash -c '"
                     "cd /tmp && rm -rf %2 && "
                     "git clone https://aur.archlinux.org/%2.git && "
                     "cd %2 && makepkg -si --noconfirm'").arg(tu, helperBin)}};
    }

    // ---- Package Managers ----
    if (get("systemtools/octopi"))
        S << pacStep("octopi",   "Install Octopi",   {"octopi"});
    if (get("systemtools/pacseek"))
        S << pacStep("pacseek",  "Install Pacseek",  {"pacseek"});
    if (get("systemtools/bazaar"))
        S << pacStep("bazaar",   "Install Bazaar",   {"bazaar"});

    // ---- Driver Extras ----
    if (get("systemtools/rocm"))
        S << pacStep("rocm", "Install AMD ROCm compute stack",
                     {"rocm-hip-sdk", "rocm-opencl-sdk"});
    if (get("systemtools/cuda"))
        S << pacStep("cuda", "Install nVidia CUDA toolkit", {"cuda"});
    if (get("systemtools/rog")) {
        S << pacStep("rog_pkgs", "Install ASUS ROG Tools",
                     {"rog-control-center", "asusctl", "supergfxctl"});
        S << InstallStep{"rog_enable", "Enable ASUS daemon services",
                         {"systemctl", "enable", "--now", "asusd", "supergfxd"}};
    }

    // ---- System upgrade (optional) ----
    if (get("setup/sysupgrade"))
        S << InstallStep{"sysupgrade", "Full system upgrade", {"pacman", "-Syu", "--noconfirm"}};

    // ---- CachyOS Kernel & SCX ----
    if (get("setup/cachyos-kernel"))
        S << pacStep("cachyos_kernel", "Install CachyOS kernel", {"linux-cachyos", "linux-cachyos-headers"});

    return S;
}

#include <QApplication>
#include <QFont>
#include <QMessageBox>
#include "mainwizard.h"
#include <unistd.h>
#include <pwd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// Sentinel prefix for display env vars we ferry through pkexec's arg list.
// pkexec strips environment but preserves argv, so we encode the display
// variables as specially-prefixed arguments and restore them on the root side.
static const char PFX_DISPLAY[]  = "--_asl_display=";
static const char PFX_WAYLAND[]  = "--_asl_wayland=";
static const char PFX_RUNTIME[]  = "--_asl_runtime=";
static const char PFX_DBUS[]     = "--_asl_dbus=";
static const char PFX_XAUTH[]    = "--_asl_xauth=";

int main(int argc, char *argv[])
{
    // ----------------------------------------------------------------
    // Privilege escalation
    // If not root: collect display env, then re-exec ourselves via pkexec
    // passing the display variables as hidden CLI arguments so Qt can
    // connect to the compositor/display after pkexec strips the environment.
    // ----------------------------------------------------------------
    if (geteuid() != 0) {
        const char *display  = std::getenv("DISPLAY");
        const char *wayland  = std::getenv("WAYLAND_DISPLAY");
        const char *runtime  = std::getenv("XDG_RUNTIME_DIR");
        const char *dbus     = std::getenv("DBUS_SESSION_BUS_ADDRESS");
        const char *xauth    = std::getenv("XAUTHORITY");

        // Prefer Wayland; fall back to X11.
        // Ensure at least one display is available before re-execing.
        if (!display && !wayland) {
            // No display at all – still try; let the user see the error from Qt.
        }

        std::vector<std::string> ferry;
        if (display) ferry.push_back(std::string(PFX_DISPLAY) + display);
        if (wayland) ferry.push_back(std::string(PFX_WAYLAND) + wayland);
        if (runtime) ferry.push_back(std::string(PFX_RUNTIME) + runtime);
        if (dbus)    ferry.push_back(std::string(PFX_DBUS)    + dbus);
        if (xauth)   ferry.push_back(std::string(PFX_XAUTH)   + xauth);

        // Build execvp argv: pkexec <this-binary> [ferry args...] [original args...]
        std::vector<char *> execArgs;
        execArgs.push_back(const_cast<char *>("pkexec"));
        execArgs.push_back(argv[0]);
        for (auto &s : ferry)
            execArgs.push_back(s.data());
        for (int i = 1; i < argc; ++i)
            execArgs.push_back(argv[i]);
        execArgs.push_back(nullptr);

        execvp("pkexec", execArgs.data());

        // execvp only returns on failure (pkexec not found / cancelled)
        // Now we can start Qt since we still have the display env.
        QApplication a(argc, argv);
        QMessageBox::critical(nullptr, "Privileges Required",
            "Arch System Loadout requires root privileges to install packages.\n\n"
            "pkexec (polkit) was not found or the authentication was cancelled.\n\n"
            "You can also run:\n"
            "  sudo -E arch-system-loadout");
        return 1;
    }

    // ----------------------------------------------------------------
    // We are root. Restore display environment from ferry args.
    // ----------------------------------------------------------------
    auto startswith = [](const char *s, const char *prefix) -> const char * {
        size_t n = strlen(prefix);
        return strncmp(s, prefix, n) == 0 ? s + n : nullptr;
    };

    // Strip our sentinel args from argv before Qt sees them.
    std::vector<char *> cleanArgv;
    cleanArgv.push_back(argv[0]);
    for (int i = 1; i < argc; ++i) {
        const char *val = nullptr;
        if      ((val = startswith(argv[i], PFX_DISPLAY))) setenv("DISPLAY",                 val, 1);
        else if ((val = startswith(argv[i], PFX_WAYLAND))) setenv("WAYLAND_DISPLAY",         val, 1);
        else if ((val = startswith(argv[i], PFX_RUNTIME))) setenv("XDG_RUNTIME_DIR",         val, 1);
        else if ((val = startswith(argv[i], PFX_DBUS)))    setenv("DBUS_SESSION_BUS_ADDRESS", val, 1);
        else if ((val = startswith(argv[i], PFX_XAUTH)))   setenv("XAUTHORITY",              val, 1);
        else cleanArgv.push_back(argv[i]);
    }
    cleanArgv.push_back(nullptr);
    int cleanArgc = static_cast<int>(cleanArgv.size()) - 1;

    // Prefer Wayland on KDE Plasma; fall back to xcb if only DISPLAY is set.
    if (std::getenv("WAYLAND_DISPLAY")) {
        setenv("QT_QPA_PLATFORM", "wayland", 0);
    } else if (std::getenv("DISPLAY")) {
        setenv("QT_QPA_PLATFORM", "xcb", 0);
    }

    // ----------------------------------------------------------------
    // Real-user environment restoration
    // pkexec sets PKEXEC_UID; sudo -E sets SUDO_USER.
    // We need the real user's HOME etc. for KDE theme loading and for
    // running user-space commands (yay, kpackagetool6 …).
    // ----------------------------------------------------------------
    struct passwd *pw = nullptr;

    const char *pkexecUid = std::getenv("PKEXEC_UID");
    const char *sudoUser  = std::getenv("SUDO_USER");

    if (pkexecUid) {
        pw = getpwuid(static_cast<uid_t>(std::atoi(pkexecUid)));
    } else if (sudoUser && std::strcmp(sudoUser, "root") != 0) {
        pw = getpwnam(sudoUser);
    }

    if (!pw) {
        FILE *f = popen("logname 2>/dev/null", "r");
        if (f) {
            char buf[256] = {};
            if (fgets(buf, sizeof(buf), f)) {
                size_t len = strlen(buf);
                if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
                if (buf[0] && strcmp(buf, "root") != 0)
                    pw = getpwnam(buf);
            }
            pclose(f);
        }
    }

    if (pw) {
        uid_t uid = pw->pw_uid;
        qputenv("HOME",             pw->pw_dir);
        qputenv("XDG_CONFIG_HOME",  (std::string(pw->pw_dir) + "/.config").c_str());
        qputenv("XDG_DATA_HOME",    (std::string(pw->pw_dir) + "/.local/share").c_str());
        // Only set runtime/dbus from uid if they weren't ferried in from the user session
        if (!std::getenv("XDG_RUNTIME_DIR") || std::string(std::getenv("XDG_RUNTIME_DIR")).empty())
            qputenv("XDG_RUNTIME_DIR", QString("/run/user/%1").arg(uid).toUtf8());
        if (!std::getenv("DBUS_SESSION_BUS_ADDRESS") || std::string(std::getenv("DBUS_SESSION_BUS_ADDRESS")).empty())
            qputenv("DBUS_SESSION_BUS_ADDRESS", QString("unix:path=/run/user/%1/bus").arg(uid).toUtf8());
    }

    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME"))
        qputenv("QT_QPA_PLATFORMTHEME", "kde");

    QApplication app(cleanArgc, cleanArgv.data());
    app.setApplicationName("Arch System Loadout");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ArchSystemLoadout");

    QFont appFont = app.font();
    appFont.setPointSize(appFont.pointSize() + 2);
    app.setFont(appFont);

    MainWizard wizard;
    wizard.show();

    return app.exec();
}

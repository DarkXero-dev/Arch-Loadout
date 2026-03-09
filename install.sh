#!/usr/bin/env bash
# Arch System Loadout - curl installer
# Usage: curl -fsSL https://raw.githubusercontent.com/USER/REPO/main/install.sh | bash
#   Or:  bash install.sh   (from within the cloned repo)

set -euo pipefail

APP="arch-system-loadout"
INSTALL_DIR="/usr/local/bin"
SRC_DIR="/tmp/$APP-src"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Arch System Loadout Installer"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Check for Arch Linux
if [ ! -f /etc/arch-release ]; then
    echo "ERROR: This application is for Arch Linux only."
    echo "       /etc/arch-release not found."
    exit 1
fi

# Check if running as root for the install phase
if [ "$(id -u)" -ne 0 ]; then
    echo "==> This installer needs root to install build dependencies and the binary."
    echo "==> Re-running with sudo..."
    exec sudo bash "$0" "$@"
fi

# Install build dependencies (only what's not already installed)
BUILD_DEPS=(cmake gcc git qt6-base)
MISSING_DEPS=()
for dep in "${BUILD_DEPS[@]}"; do
    if ! pacman -Qq "$dep" &>/dev/null; then
        MISSING_DEPS+=("$dep")
    fi
done

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo "==> Installing build dependencies: ${MISSING_DEPS[*]}"
    pacman -S --noconfirm --needed "${MISSING_DEPS[@]}"
fi

# Determine source directory
# If this script is run from inside the cloned repo, build in place.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-/dev/stdin}")" 2>/dev/null && pwd || echo "/tmp")"

if [ -f "$SCRIPT_DIR/CMakeLists.txt" ] && grep -q "arch-system-loadout" "$SCRIPT_DIR/CMakeLists.txt" 2>/dev/null; then
    echo "==> Building from local source: $SCRIPT_DIR"
    SRC_DIR="$SCRIPT_DIR"
else
    echo "==> Fetching source from GitHub..."
    rm -rf "$SRC_DIR"
    git clone https://github.com/USER/REPO.git "$SRC_DIR"
fi

# Build
echo "==> Building $APP..."
cd "$SRC_DIR"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
make -j"$(nproc)"

# Install
echo "==> Installing to $INSTALL_DIR..."
make install

# Verify
if command -v "$APP" &>/dev/null || [ -x "$INSTALL_DIR/$APP" ]; then
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  Installation complete!"
    echo "  Launch with: pkexec $APP"
    echo "  Or from your application menu."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "==> Launching $APP..."
    exec pkexec "$INSTALL_DIR/$APP"
else
    echo "ERROR: Installation failed — binary not found at $INSTALL_DIR/$APP"
    exit 1
fi

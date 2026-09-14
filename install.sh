#!/usr/bin/env bash
# ==============================================================================
# emoji-picker installer
# One-step installer with multi-condition checks and desktop shortcut setup.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Defaults
PREFIX="${PREFIX:-$HOME/.local}"
BINDIR="$PREFIX/bin"
SYSTEMD_USER_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
SERVICE_DROPIN_DIR="$SYSTEMD_USER_DIR/emoji-picker.service.d"
DRY_RUN=0
NO_BUILD=0
NO_SERVICE=0
NO_SHORTCUT=0
UNINSTALL=0
CHECK_ONLY=0
DO_UPDATE=0
CHECK_UPDATE=0

# Colors for output
if [[ -t 1 ]]; then
  C_RESET="\033[0m"
  C_BOLD="\033[1m"
  C_GREEN="\033[32m"
  C_YELLOW="\033[33m"
  C_RED="\033[31m"
  C_CYAN="\033[36m"
else
  C_RESET=""
  C_BOLD=""
  C_GREEN=""
  C_YELLOW=""
  C_RED=""
  C_CYAN=""
fi

log_info()    { echo -e "${C_CYAN}ℹ${C_RESET} $*"; }
log_success() { echo -e "${C_GREEN}✔${C_RESET} $*"; }
log_warn()    { echo -e "${C_YELLOW}⚠${C_RESET} $*"; }
log_error()   { echo -e "${C_RED}✖${C_RESET} $*" >&2; }
log_step()    { echo -e "\n${C_BOLD}${C_CYAN}==>${C_RESET} ${C_BOLD}$*${C_RESET}"; }

usage() {
  cat <<EOF
Usage: $0 [OPTIONS]

One-step installer for emoji-picker on Linux X11 desktops.
Builds the project (if needed), installs binary and systemd service,
and automatically registers the Super+Period (Win+.) shortcut for
LXQt, Openbox, GNOME, XFCE, i3, and Sway.

Options:
  -h, --help        Show this help message and exit
  --dry-run         Show actions without making changes
  --uninstall       Uninstall emoji-picker, systemd service, and configurations
  --check           Check system requirements and exit
  --update          Update to latest version (via git or GitHub releases)
  --check-update    Check for available updates without modifying anything
  --no-build        Skip compilation (use existing binary)
  --no-service      Skip systemd user service installation
  --no-shortcut     Skip desktop shortcut configuration
  --prefix=<dir>    Install prefix (default: ~/.local)

Examples:
  ./install.sh                  # Complete standard installation
  ./install.sh --update         # Upgrade to latest release
  ./install.sh --check-update   # Check if new version exists
  ./install.sh --dry-run        # Preview steps
  ./install.sh --uninstall      # Clean removal
EOF
}

# Parse arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --dry-run)
      DRY_RUN=1
      shift
      ;;
    --uninstall)
      UNINSTALL=1
      shift
      ;;
    --check)
      CHECK_ONLY=1
      shift
      ;;
    --update)
      DO_UPDATE=1
      shift
      ;;
    --check-update)
      CHECK_UPDATE=1
      shift
      ;;
    --no-build)
      NO_BUILD=1
      shift
      ;;
    --no-service)
      NO_SERVICE=1
      shift
      ;;
    --no-shortcut)
      NO_SHORTCUT=1
      shift
      ;;
    --prefix=*)
      PREFIX="${1#*=}"
      BINDIR="$PREFIX/bin"
      shift
      ;;
    *)
      log_error "Unknown option: $1"
      usage
      exit 1
      ;;
  esac
done

detect_pkg_manager_cmd() {
  if command -v apt-get >/dev/null 2>&1; then
    echo "sudo apt install -y build-essential pkg-config libgtk-3-dev libx11-dev xdotool fonts-noto-color-emoji xclip"
  elif command -v dnf >/dev/null 2>&1; then
    echo "sudo dnf install -y gcc make pkg-config gtk3-devel libX11-devel xdotool google-noto-color-emoji-fonts xclip"
  elif command -v pacman >/dev/null 2>&1; then
    echo "sudo pacman -S --needed base-devel gtk3 libx11 xdotool noto-fonts-emoji xclip"
  elif command -v zypper >/dev/null 2>&1; then
    echo "sudo zypper install -y gcc make pkg-config gtk3-devel libX11-devel xdotool noto-coloremoji-fonts xclip"
  elif command -v apk >/dev/null 2>&1; then
    echo "sudo apk add build-base pkgconf gtk+3.0-dev libx11-dev xdotool font-noto-emoji xclip"
  else
    echo "Please install GTK3 development libraries, X11, xdotool, xclip, and color emoji fonts using your package manager."
  fi
}

check_runtime_deps() {
  local missing=()
  for cmd in xdotool xclip; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
      missing+=("$cmd")
    fi
  done

  # Check for emoji font availability if fc-list is available
  local has_emoji_font=1
  if command -v fc-list >/dev/null 2>&1; then
    if ! fc-list : family | grep -qi "emoji"; then
      has_emoji_font=0
      missing+=("color-emoji-font (e.g. fonts-noto-color-emoji)")
    fi
  fi

  if [[ ${#missing[@]} -gt 0 ]]; then
    log_warn "Missing runtime dependencies: ${missing[*]}"
    log_info "Suggested installation command:"
    echo "    $(detect_pkg_manager_cmd)"
    return 1
  fi
  log_success "All runtime dependencies found (xdotool, xclip, emoji font)"
  return 0
}

check_build_deps() {
  local missing=()
  for cmd in gcc make pkg-config; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
      missing+=("$cmd")
    fi
  done

  if command -v pkg-config >/dev/null 2>&1; then
    if ! pkg-config --exists gtk+-3.0; then
      missing+=("libgtk-3-dev")
    fi
    if ! pkg-config --exists x11; then
      missing+=("libx11-dev")
    fi
  fi

  if [[ ${#missing[@]} -gt 0 ]]; then
    log_warn "Missing build dependencies: ${missing[*]}"
    log_info "Suggested installation command:"
    echo "    $(detect_pkg_manager_cmd)"
    return 1
  fi
  log_success "All build tools found (gcc, make, pkg-config, gtk+-3.0, x11)"
  return 0
}

do_uninstall() {
  log_step "Uninstalling emoji-picker"

  # 1. Stop and disable systemd service
  if command -v systemctl >/dev/null 2>&1; then
    if systemctl --user is-active emoji-picker.service >/dev/null 2>&1; then
      log_info "Stopping systemd user service..."
      [[ $DRY_RUN -eq 0 ]] && systemctl --user stop emoji-picker.service || true
    fi
    if systemctl --user is-enabled emoji-picker.service >/dev/null 2>&1; then
      log_info "Disabling systemd user service..."
      [[ $DRY_RUN -eq 0 ]] && systemctl --user disable emoji-picker.service || true
    fi
  fi

  # 2. Remove files
  local service_file="$SYSTEMD_USER_DIR/emoji-picker.service"
  local dropin_file="$SERVICE_DROPIN_DIR/display.conf"
  local bin_file="$BINDIR/emoji-picker"

  for f in "$service_file" "$dropin_file" "$bin_file"; do
    if [[ -f "$f" ]]; then
      log_info "Removing $f"
      [[ $DRY_RUN -eq 0 ]] && rm -f "$f"
    fi
  done

  if [[ -d "$SERVICE_DROPIN_DIR" ]]; then
    [[ $DRY_RUN -eq 0 ]] && rmdir "$SERVICE_DROPIN_DIR" 2>/dev/null || true
  fi

  if command -v systemctl >/dev/null 2>&1 && [[ $DRY_RUN -eq 0 ]]; then
    systemctl --user daemon-reload || true
  fi

  # 3. Clean shortcuts from Openbox / LXQt if present
  local ob_conf="${XDG_CONFIG_HOME:-$HOME/.config}/openbox/rc.xml"
  if [[ -f "$ob_conf" ]] && grep -q "emoji-picker" "$ob_conf"; then
    log_info "Found emoji-picker shortcut in Openbox rc.xml. You may want to review $ob_conf"
  fi

  local lxqt_conf="${XDG_CONFIG_HOME:-$HOME/.config}/lxqt/globalkeyshortcuts.conf"
  if [[ -f "$lxqt_conf" ]] && grep -q "emoji-picker" "$lxqt_conf"; then
    log_info "Found emoji-picker shortcut in LXQt globalkeyshortcuts.conf. You may want to review $lxqt_conf"
  fi

  log_success "Uninstallation completed."
}

build_if_needed() {
  if [[ -f "emoji-picker" ]]; then
    log_success "Pre-built binary 'emoji-picker' already exists."
    return 0
  fi

  if [[ $NO_BUILD -eq 1 ]]; then
    log_error "Binary 'emoji-picker' not found and --no-build was specified."
    exit 1
  fi

  log_step "Building emoji-picker"
  if ! check_build_deps; then
    log_error "Cannot build without required build dependencies."
    exit 1
  fi

  # Check if generated files exist, if not run make data
  if [[ ! -f "src/generated/emoji_data.h" || ! -f "src/generated/emoji_data.c" ]]; then
    log_info "Generating emoji dataset ('make data')..."
    if [[ $DRY_RUN -eq 1 ]]; then
      log_info "[dry-run] Would run: make data"
    else
      make data
    fi
  fi

  log_info "Compiling binary ('make')..."
  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would run: make"
  else
    make
  fi
  log_success "Build complete."
}

install_binary() {
  log_step "Installing binary to $BINDIR"
  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would create directory: $BINDIR"
    log_info "[dry-run] Would install emoji-picker -> $BINDIR/emoji-picker"
  else
    mkdir -p "$BINDIR"
    install -m 755 emoji-picker "$BINDIR/emoji-picker"
  fi
  log_success "Installed $BINDIR/emoji-picker"

  # PATH check
  if [[ ":$PATH:" != *":$BINDIR:"* ]]; then
    log_warn "$BINDIR is not in your current PATH."
    log_info "Add this to your ~/.bashrc or ~/.zshrc:"
    echo "    export PATH=\"$BINDIR:\$PATH\""
  fi
}

install_systemd_service() {
  if [[ $NO_SERVICE -eq 1 ]]; then
    log_info "Skipping systemd service setup (--no-service)."
    return 0
  fi

  if ! command -v systemctl >/dev/null 2>&1; then
    log_warn "systemctl not found; skipping user service setup."
    return 0
  fi

  log_step "Configuring systemd user service"

  local service_src="pack/emoji-picker.service"
  if [[ ! -f "$service_src" ]]; then
    if [[ -f "emoji-picker.service" ]]; then
      service_src="emoji-picker.service"
    else
      log_error "Service unit file not found (checked pack/emoji-picker.service and ./emoji-picker.service)."
      return 1
    fi
  fi

  local target_service="$SYSTEMD_USER_DIR/emoji-picker.service"
  local dropin_file="$SERVICE_DROPIN_DIR/display.conf"

  # Current display & xauth detection
  local cur_display="${DISPLAY:-:0}"
  local cur_xauth="${XAUTHORITY:-$HOME/.Xauthority}"

  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would install $service_src -> $target_service"
    log_info "[dry-run] Would configure $dropin_file with DISPLAY=$cur_display and XAUTHORITY=$cur_xauth"
    log_info "[dry-run] Would run: systemctl --user daemon-reload"
    log_info "[dry-run] Would run: systemctl --user enable --now emoji-picker.service"
  else
    mkdir -p "$SYSTEMD_USER_DIR"
    install -m 644 "$service_src" "$target_service"

    mkdir -p "$SERVICE_DROPIN_DIR"
    cat > "$dropin_file" <<EOF
[Service]
Environment=DISPLAY=${cur_display}
Environment=XAUTHORITY=${cur_xauth}
EOF

    systemctl --user daemon-reload
    systemctl --user enable emoji-picker.service
    systemctl --user restart emoji-picker.service
  fi

  log_success "systemd user service installed and enabled."
}

# --- Shortcut Configuration Helpers ---

configure_openbox_shortcut() {
  local target_bin="$1"
  local ob_conf="${XDG_CONFIG_HOME:-$HOME/.config}/openbox/rc.xml"
  if [[ ! -f "$ob_conf" ]]; then
    ob_conf="${XDG_CONFIG_HOME:-$HOME/.config}/openbox/lxqt-rc.xml"
  fi

  if [[ ! -f "$ob_conf" ]]; then
    return 1
  fi

  log_info "Checking Openbox configuration: $ob_conf"

  # Check if already configured
  if grep -q "W-period" "$ob_conf" || grep -q "emoji-picker.*--toggle" "$ob_conf"; then
    log_success "Openbox: Win+. shortcut already present in $ob_conf."
    return 0
  fi

  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would add <keybind key=\"W-period\"> to Openbox config: $ob_conf"
    return 0
  fi

  # Backup
  cp "$ob_conf" "${ob_conf}.bak"

  # Insert keybind before closing </keyboard>
  local keybind_xml="    <!-- Super+Period / Win+. — emoji-picker -->\n    <keybind key=\"W-period\">\n      <action name=\"Execute\">\n        <command>${target_bin} --toggle</command>\n      </action>\n    </keybind>"

  if grep -q "</keyboard>" "$ob_conf"; then
    awk -v snippet="$keybind_xml" '
      /<\/keyboard>/ {
        print snippet
      }
      { print }
    ' "${ob_conf}.bak" > "$ob_conf"
    log_success "Openbox: Added Win+. shortcut to $ob_conf (backup: ${ob_conf}.bak)."

    if [[ -z "${MOCK_TEST:-}" ]] && pgrep -x openbox >/dev/null 2>&1; then
      openbox --reconfigure >/dev/null 2>&1 || true
      log_success "Openbox: reconfigured active session."
    fi
    return 0
  else
    log_warn "Openbox: No </keyboard> closing tag found in $ob_conf. Please bind manually."
    return 1
  fi
}

configure_lxqt_shortcut() {
  local target_bin="$1"
  local lxqt_conf="${XDG_CONFIG_HOME:-$HOME/.config}/lxqt/globalkeyshortcuts.conf"

  if [[ ! -f "$lxqt_conf" ]]; then
    return 1
  fi

  log_info "Checking LXQt configuration: $lxqt_conf"

  if grep -q "\[Meta%2Bperiod" "$lxqt_conf" || grep -q "emoji-picker.*--toggle" "$lxqt_conf"; then
    log_success "LXQt: Win+. shortcut already present in $lxqt_conf."
    return 0
  fi

  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would add [Meta%2Bperiod.N] to LXQt config: $lxqt_conf"
    return 0
  fi

  # Backup
  cp "$lxqt_conf" "${lxqt_conf}.bak"

  # Find highest section number
  local max_num
  max_num=$(grep -oE '\[[^]]+\.[0-9]+\]' "$lxqt_conf" | sed -E 's/.*\.([0-9]+)\]/\1/' | sort -n | tail -n1 || echo "0")
  if [[ -z "$max_num" ]]; then
    max_num=0
  fi
  local next_num=$((max_num + 1))

  cat >> "$lxqt_conf" <<EOF

[Meta%2Bperiod.${next_num}]
Comment=Emoji picker
Enabled=true
Exec=${target_bin}, --toggle
EOF

  log_success "LXQt: Added [Meta%2Bperiod.${next_num}] to $lxqt_conf (backup: ${lxqt_conf}.bak)."

  if [[ -z "${MOCK_TEST:-}" ]] && (pgrep -x lxqt-globalkeys >/dev/null 2>&1 || pgrep -x lxqt-globalkeysd >/dev/null 2>&1); then
    killall lxqt-globalkeysd 2>/dev/null || killall lxqt-globalkeys 2>/dev/null || true
    nohup lxqt-globalkeysd >/dev/null 2>&1 &
    log_success "LXQt: Reloaded lxqt-globalkeysd."
  fi
  return 0
}

configure_gnome_shortcut() {
  local target_bin="$1"
  if ! command -v gsettings >/dev/null 2>&1; then
    return 1
  fi

  local schema="org.gnome.settings-daemon.plugins.media-keys"
  if ! gsettings list-schemas 2>/dev/null | grep -q "^$schema$"; then
    return 1
  fi

  log_info "Configuring GNOME shortcut via gsettings..."
  local path="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/emoji-picker/"
  local child_schema="org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$path"

  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would register GNOME custom keybinding for Super+period"
    return 0
  fi

  local current_list
  current_list=$(gsettings get "$schema" custom-keybindings 2>/dev/null || echo "@as []")

  if [[ "$current_list" != *"$path"* ]]; then
    local new_list
    if [[ "$current_list" == "@as []" || "$current_list" == "[]" ]]; then
      new_list="['$path']"
    else
      new_list="${current_list%]*}, '$path']"
    fi
    gsettings set "$schema" custom-keybindings "$new_list"
  fi

  gsettings set "$child_schema" name "Emoji Picker"
  gsettings set "$child_schema" command "$target_bin --toggle"
  gsettings set "$child_schema" binding "<Super>period"
  log_success "GNOME: Registered Super+Period shortcut via gsettings."
  return 0
}

configure_xfce_shortcut() {
  local target_bin="$1"
  if ! command -v xfconf-query >/dev/null 2>&1; then
    return 1
  fi

  log_info "Configuring XFCE shortcut via xfconf-query..."
  local prop="/commands/custom/<Super>period"

  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would register XFCE shortcut: $prop -> $target_bin --toggle"
    return 0
  fi

  xfconf-query -c xfce4-keyboard-shortcuts -p "$prop" -n -t string -s "$target_bin --toggle" 2>/dev/null || \
  xfconf-query -c xfce4-keyboard-shortcuts -p "$prop" -s "$target_bin --toggle" 2>/dev/null || true
  log_success "XFCE: Registered <Super>period shortcut."
  return 0
}

configure_i3_sway_shortcut() {
  local target_bin="$1"
  local config_file=""

  if [[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/i3/config" ]]; then
    config_file="${XDG_CONFIG_HOME:-$HOME/.config}/i3/config"
  elif [[ -f "$HOME/.i3/config" ]]; then
    config_file="$HOME/.i3/config"
  elif [[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/sway/config" ]]; then
    config_file="${XDG_CONFIG_HOME:-$HOME/.config}/sway/config"
  fi

  if [[ -z "$config_file" ]]; then
    return 1
  fi

  log_info "Checking i3/sway configuration: $config_file"
  if grep -q "emoji-picker.*--toggle" "$config_file"; then
    log_success "i3/sway: Shortcut already present in $config_file."
    return 0
  fi

  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would append bindsym to $config_file"
    return 0
  fi

  cp "$config_file" "${config_file}.bak"
  echo -e "\n# Emoji picker toggle\nbindsym \$mod+period exec --no-startup-id $target_bin --toggle" >> "$config_file"
  log_success "i3/sway: Added shortcut to $config_file."

  if pgrep -x i3 >/dev/null 2>&1 && command -v i3-msg >/dev/null 2>&1; then
    i3-msg reload >/dev/null 2>&1 || true
  elif pgrep -x sway >/dev/null 2>&1 && command -v swaymsg >/dev/null 2>&1; then
    swaymsg reload >/dev/null 2>&1 || true
  fi
  return 0
}

configure_shortcuts() {
  if [[ $NO_SHORTCUT -eq 1 ]]; then
    log_info "Skipping shortcut setup (--no-shortcut)."
    return 0
  fi

  log_step "Configuring global shortcut (Win+. / Super+Period)"
  local target_bin="$BINDIR/emoji-picker"
  local configured=0

  # Check Openbox
  if configure_openbox_shortcut "$target_bin"; then
    configured=1
  fi

  # Check LXQt (Note: LXQt + Openbox often coexist on Lubuntu; bind both!)
  if configure_lxqt_shortcut "$target_bin"; then
    configured=1
  fi

  # Check GNOME
  if configure_gnome_shortcut "$target_bin"; then
    configured=1
  fi

  # Check XFCE
  if configure_xfce_shortcut "$target_bin"; then
    configured=1
  fi

  # Check i3 / Sway
  if configure_i3_sway_shortcut "$target_bin"; then
    configured=1
  fi

  if [[ $configured -eq 0 ]]; then
    log_warn "Could not automatically register shortcut for your desktop environment."
    log_info "Please manually set a global shortcut in your desktop settings:"
    echo "    Key:     Super + Period (Win + .)"
    echo "    Command: $target_bin --toggle"
  else
    log_success "Shortcut configuration completed for active desktop environment(s)."
  fi
}

verify_installation() {
  log_step "Verifying installation"

  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Skipping verification."
    return 0
  fi

  # Verify binary
  if [[ ! -x "$BINDIR/emoji-picker" ]]; then
    log_error "Binary $BINDIR/emoji-picker is not executable or missing."
    return 1
  fi
  log_success "Binary is ready at $BINDIR/emoji-picker"

  # Verify systemd service status
  if [[ $NO_SERVICE -eq 0 ]] && command -v systemctl >/dev/null 2>&1; then
    if systemctl --user is-active emoji-picker.service >/dev/null 2>&1; then
      log_success "Daemon service 'emoji-picker.service' is running actively."
    else
      log_warn "Daemon service 'emoji-picker.service' is not active yet."
      log_info "Check logs with: journalctl --user -u emoji-picker.service -n 20"
    fi
  fi
}

CURRENT_REPO="habibiahmada/emoji-picker"

run_with_timeout() {
  local timeout_sec="$1"
  shift
  if command -v timeout >/dev/null 2>&1; then
    timeout "$timeout_sec" "$@"
  else
    "$@"
  fi
}

get_installed_version() {
  local ver=""
  if [[ -x "$BINDIR/emoji-picker" ]]; then
    ver=$(run_with_timeout 2s "$BINDIR/emoji-picker" --version 2>/dev/null | awk '{print $2}' || true)
  fi
  if [[ -z "$ver" ]] && [[ -x "./emoji-picker" ]]; then
    ver=$(run_with_timeout 2s ./emoji-picker --version 2>/dev/null | awk '{print $2}' || true)
  fi
  if [[ -z "$ver" ]]; then
    ver="0.3.0"
  fi
  echo "${ver#v}"
}

get_latest_github_release() {
  if [[ -n "${MOCK_LATEST_VERSION:-}" ]]; then
    echo "${MOCK_LATEST_VERSION#v}"
    return 0
  fi
  if ! command -v curl >/dev/null 2>&1; then
    return 1
  fi
  local api_url="https://api.github.com/repos/${CURRENT_REPO}/releases/latest"
  local json
  json=$(curl -sSf --connect-timeout 2 --max-time 3 "$api_url" 2>/dev/null || true)
  if [[ -n "$json" ]]; then
    local tag
    tag=$(echo "$json" | grep -o '"tag_name": *"[^"]*"' | head -n1 | cut -d'"' -f4 || true)
    if [[ -n "$tag" ]]; then
      echo "${tag#v}"
      return 0
    fi
  fi
  return 1
}

version_gt() {
  # returns 0 (true) if $1 > $2
  test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"
}

do_check_update() {
  log_step "Checking for updates"
  local current_ver
  current_ver=$(get_installed_version)
  log_info "Current installed version: v${current_ver}"

  local update_available=0
  local latest_found=""
  local checked_remote=0

  # Mode A: check git repository if applicable
  if [[ -z "${MOCK_LATEST_VERSION:-}" ]] && [[ -d ".git" ]] && command -v git >/dev/null 2>&1; then
    log_info "Checking git remote origin..."
    export GIT_TERMINAL_PROMPT=0
    export GIT_SSH_COMMAND="ssh -o BatchMode=yes -o ConnectTimeout=2"
    if run_with_timeout 2s git fetch --tags origin >/dev/null 2>&1; then
      checked_remote=1
      local local_head
      local_head=$(git rev-parse HEAD 2>/dev/null || echo "")
      local branch
      branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "main")
      local remote_head
      remote_head=$(git rev-parse "origin/$branch" 2>/dev/null || echo "")
      local latest_tag
      latest_tag=$(git tag -l 'v*' | sort -V | tail -n1 || echo "")
      latest_tag="${latest_tag#v}"

      if [[ -n "$remote_head" && "$local_head" != "$remote_head" ]]; then
        update_available=1
        latest_found="latest commits on origin/$branch"
      elif [[ -n "$latest_tag" ]] && version_gt "$latest_tag" "$current_ver"; then
        update_available=1
        latest_found="v${latest_tag}"
      fi
    fi
  fi

  # Mode B: check GitHub Releases API
  if [[ $update_available -eq 0 ]]; then
    local gh_release
    if gh_release=$(get_latest_github_release); then
      checked_remote=1
      if version_gt "$gh_release" "$current_ver"; then
        update_available=1
        latest_found="v${gh_release}"
      fi
    fi
  fi

  if [[ $update_available -eq 1 ]]; then
    log_success "An update is available: ${latest_found}! 🎉"
    log_info "Run './install.sh --update' (or 'make update') to update."
  elif [[ $checked_remote -eq 1 ]]; then
    log_success "You are already using the latest version (v${current_ver})."
  else
    log_warn "Could not reach remote server to check for updates (offline / network timeout)."
    log_info "Current installed version remains v${current_ver}."
  fi
  return 0
}

do_update() {
  log_step "Updating emoji-picker"
  local current_ver
  current_ver=$(get_installed_version)
  log_info "Current installed version: v${current_ver}"

  # Mode A: Git repository
  if [[ -d ".git" ]] && command -v git >/dev/null 2>&1; then
    log_info "Updating via git repository..."
    if [[ $DRY_RUN -eq 1 ]]; then
      log_info "[dry-run] Would pull latest commits from git remote"
      log_info "[dry-run] Would rebuild dataset: make data"
      log_info "[dry-run] Would compile: make"
      log_info "[dry-run] Would reinstall binary and restart service"
      return 0
    fi

    # Stash uncommitted changes if any
    local stashed=0
    if ! git diff-index --quiet HEAD -- 2>/dev/null; then
      log_warn "Local modifications detected. Saving to git stash..."
      git stash >/dev/null 2>&1 || true
      stashed=1
    fi

    local branch
    branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "main")
    log_info "Pulling updates from origin/$branch..."
    if ! git pull --ff-only origin "$branch" 2>/dev/null; then
      log_warn "Fast-forward merge not possible. Fetching and rebasing..."
      git fetch origin "$branch"
      git rebase "origin/$branch" || {
        log_error "Failed to cleanly rebase. Please resolve git conflicts manually."
        exit 1
      }
    fi

    if [[ $stashed -eq 1 ]]; then
      log_info "Restoring stashed changes..."
      git stash pop >/dev/null 2>&1 || true
    fi

    log_info "Refreshing Unicode emoji data (make data)..."
    make data

    log_info "Recompiling binary (make)..."
    make clean
    make

    install_binary
    install_systemd_service
    verify_installation

    local new_ver
    new_ver=$(get_installed_version)
    log_success "Successfully updated to v${new_ver}! 🎉"
    return 0
  fi

  # Mode B: Standalone / GitHub Release Tarball
  log_info "Updating via GitHub Releases..."
  local latest_release
  if ! latest_release=$(get_latest_github_release); then
    log_error "Could not connect to GitHub API or determine latest release."
    exit 1
  fi

  if ! version_gt "$latest_release" "$current_ver"; then
    log_success "Already on the latest release (v${current_ver})."
    return 0
  fi

  local arch
  arch=$(uname -m)
  case "$arch" in
    x86_64|amd64) arch="x86_64" ;;
    aarch64|arm64) arch="aarch64" ;;
    *) log_error "Unsupported architecture for release tarball: $arch"; exit 1 ;;
  esac

  local tarball_name="emoji-picker-${latest_release}-linux-${arch}.tar.gz"
  local download_url="https://github.com/${CURRENT_REPO}/releases/download/v${latest_release}/${tarball_name}"

  log_info "Downloading release v${latest_release} from $download_url..."
  if [[ $DRY_RUN -eq 1 ]]; then
    log_info "[dry-run] Would download $download_url and execute installer"
    return 0
  fi

  local tmp_dir
  tmp_dir=$(mktemp -d)
  trap 'rm -rf "$tmp_dir"' EXIT

  if command -v curl >/dev/null 2>&1; then
    curl -sSLf -o "$tmp_dir/$tarball_name" "$download_url"
  elif command -v wget >/dev/null 2>&1; then
    wget -qO "$tmp_dir/$tarball_name" "$download_url"
  else
    log_error "Neither curl nor wget is available for downloading updates."
    exit 1
  fi

  tar -xzf "$tmp_dir/$tarball_name" -C "$tmp_dir"
  local extracted_dir="$tmp_dir/emoji-picker-${latest_release}-linux-${arch}"

  if [[ -f "$extracted_dir/install.sh" ]]; then
    chmod +x "$extracted_dir/install.sh"
    "$extracted_dir/install.sh" --prefix="$PREFIX" --no-shortcut
  else
    mkdir -p "$BINDIR"
    install -m 755 "$extracted_dir/emoji-picker" "$BINDIR/emoji-picker"
    install_systemd_service
  fi

  verify_installation
  log_success "Successfully updated to v${latest_release}! 🎉"
}

main() {
  echo -e "${C_BOLD}emoji-picker installer${C_RESET}"
  echo "Target directory: $BINDIR"

  if [[ $UNINSTALL -eq 1 ]]; then
    do_uninstall
    exit 0
  fi

  if [[ $CHECK_UPDATE -eq 1 ]]; then
    do_check_update
    exit 0
  fi

  if [[ $DO_UPDATE -eq 1 ]]; then
    do_update
    exit 0
  fi

  log_step "Checking system requirements"
  check_runtime_deps || true

  if [[ $CHECK_ONLY -eq 1 ]]; then
    check_build_deps || true
    log_info "System check complete."
    exit 0
  fi

  build_if_needed
  install_binary
  install_systemd_service
  configure_shortcuts
  verify_installation

  echo ""
  echo -e "${C_BOLD}${C_GREEN}====================================================${C_RESET}"
  echo -e "${C_BOLD}${C_GREEN} emoji-picker installed successfully! 🎉${C_RESET}"
  echo -e "${C_BOLD}${C_GREEN}====================================================${C_RESET}"
  echo ""
  echo -e "Press ${C_BOLD}Win + .${C_RESET} (Super + Period) anywhere to toggle the emoji picker!"
  echo -e "Command line toggle: ${C_CYAN}$BINDIR/emoji-picker --toggle${C_RESET}"
  echo ""
}

# Run main unless sourced for testing
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main
fi

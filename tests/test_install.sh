#!/usr/bin/env bash
# ==============================================================================
# tests/test_install.sh
# Test suite for install.sh installer logic and desktop shortcut configurations
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
INSTALLER="$ROOT_DIR/install.sh"
export MOCK_TEST=1

PASS_COUNT=0
FAIL_COUNT=0

assert_eq() {
  local expected="$1"
  local actual="$2"
  local msg="$3"
  if [[ "$expected" == "$actual" ]]; then
    echo "  ✔ $msg"
    PASS_COUNT=$((PASS_COUNT + 1))
  else
    echo "  ✖ $msg (expected '$expected', got '$actual')" >&2
    FAIL_COUNT=$((FAIL_COUNT + 1))
  fi
}

assert_contains() {
  local needle="$1"
  local haystack="$2"
  local msg="$3"
  if [[ "$haystack" == *"$needle"* ]]; then
    echo "  ✔ $msg"
    PASS_COUNT=$((PASS_COUNT + 1))
  else
    echo "  ✖ $msg (did not find '$needle')" >&2
    FAIL_COUNT=$((FAIL_COUNT + 1))
  fi
}

echo "=== Running install.sh Test Suite ==="

# 1. Test CLI Help
echo "[Test 1] CLI --help flag"
output=$("$INSTALLER" --help)
assert_contains "Usage:" "$output" "--help should print usage"
assert_contains "--dry-run" "$output" "--help should list --dry-run option"

# 2. Test Invalid Option
echo "[Test 2] CLI invalid option handling"
set +e
"$INSTALLER" --invalid-flag-12345 >/dev/null 2>&1
status=$?
set -e
assert_eq "1" "$status" "Invalid flag should return exit status 1"

# 3. Test System Check Mode
echo "[Test 3] CLI --check mode"
output=$("$INSTALLER" --check)
assert_contains "System check complete." "$output" "--check should run system check and exit"

# 4. Test Dry-Run with custom prefix
echo "[Test 4] CLI --dry-run with custom prefix"
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

output=$("$INSTALLER" --dry-run --prefix="$TMP_DIR/local")
assert_contains "[dry-run]" "$output" "--dry-run should indicate dry-run actions"
# Verify no files actually created under TMP_DIR in dry-run
file_count=$(find "$TMP_DIR" -type f | wc -l)
assert_eq "0" "$file_count" "Dry run should not create any files on disk"

# 5. Test Openbox shortcut injection and idempotency
echo "[Test 5] Openbox shortcut injection & idempotency"
MOCK_HOME="$TMP_DIR/mock_home"
mkdir -p "$MOCK_HOME/.config/openbox"
OB_CONF="$MOCK_HOME/.config/openbox/rc.xml"

cat > "$OB_CONF" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<openbox_config xmlns="http://openbox.org/3.4/rc">
  <keyboard>
    <keybind key="A-F4">
      <action name="Close"/>
    </keybind>
  </keyboard>
</openbox_config>
EOF

# Source install.sh in subshell with MOCK environment to test configure_openbox_shortcut
(
  export XDG_CONFIG_HOME="$MOCK_HOME/.config"
  export HOME="$MOCK_HOME"
  # shellcheck source=/dev/null
  source "$INSTALLER" --no-service --no-build --dry-run >/dev/null 2>&1 || true
  DRY_RUN=0
  configure_openbox_shortcut "$MOCK_HOME/.local/bin/emoji-picker"
)

# Verify keybind inserted
ob_content=$(cat "$OB_CONF")
assert_contains "<keybind key=\"W-period\">" "$ob_content" "Openbox rc.xml should contain W-period keybind"
assert_contains "$MOCK_HOME/.local/bin/emoji-picker --toggle" "$ob_content" "Openbox rc.xml should contain command"

# Test Idempotency: run configure_openbox_shortcut a second time
(
  export XDG_CONFIG_HOME="$MOCK_HOME/.config"
  export HOME="$MOCK_HOME"
  # shellcheck source=/dev/null
  source "$INSTALLER" --no-service --no-build --dry-run >/dev/null 2>&1 || true
  DRY_RUN=0
  configure_openbox_shortcut "$MOCK_HOME/.local/bin/emoji-picker"
)

keybind_occurrences=$(grep -c "<keybind key=\"W-period\">" "$OB_CONF")
assert_eq "1" "$keybind_occurrences" "Openbox keybind should only appear once (idempotent)"

# 6. Test LXQt shortcut injection and idempotency
echo "[Test 6] LXQt shortcut injection & idempotency"
mkdir -p "$MOCK_HOME/.config/lxqt"
LXQT_CONF="$MOCK_HOME/.config/lxqt/globalkeyshortcuts.conf"

cat > "$LXQT_CONF" <<'EOF'
[Alt%2BF4.10]
Comment=Close window
Enabled=true
Exec=openbox, --close

[Control%2Bq.11]
Comment=Quit
Enabled=true
Exec=true
EOF

(
  export XDG_CONFIG_HOME="$MOCK_HOME/.config"
  export HOME="$MOCK_HOME"
  # shellcheck source=/dev/null
  source "$INSTALLER" --no-service --no-build --dry-run >/dev/null 2>&1 || true
  DRY_RUN=0
  configure_lxqt_shortcut "$MOCK_HOME/.local/bin/emoji-picker"
)

lxqt_content=$(cat "$LXQT_CONF")
assert_contains "[Meta%2Bperiod.12]" "$lxqt_content" "LXQt config should have section [Meta%2Bperiod.12]"
assert_contains "Exec=$MOCK_HOME/.local/bin/emoji-picker, --toggle" "$lxqt_content" "LXQt config should have Exec with toggle"

# Test Idempotency: run configure_lxqt_shortcut a second time
(
  export XDG_CONFIG_HOME="$MOCK_HOME/.config"
  export HOME="$MOCK_HOME"
  # shellcheck source=/dev/null
  source "$INSTALLER" --no-service --no-build --dry-run >/dev/null 2>&1 || true
  DRY_RUN=0
  configure_lxqt_shortcut "$MOCK_HOME/.local/bin/emoji-picker"
)

lxqt_occurrences=$(grep -c "Meta%2Bperiod" "$LXQT_CONF")
assert_eq "1" "$lxqt_occurrences" "LXQt keybind section should only appear once (idempotent)"

# 7. Test Systemd service & drop-in file generation
echo "[Test 7] Systemd service and display.conf generation"
(
  export XDG_CONFIG_HOME="$MOCK_HOME/.config"
  export HOME="$MOCK_HOME"
  export DISPLAY=":42"
  export XAUTHORITY="$MOCK_HOME/.test_xauth"
  SYSTEMD_USER_DIR="$MOCK_HOME/.config/systemd/user"
  SERVICE_DROPIN_DIR="$SYSTEMD_USER_DIR/emoji-picker.service.d"
  # shellcheck source=/dev/null
  source "$INSTALLER" --no-service --no-build --dry-run >/dev/null 2>&1 || true
  DRY_RUN=0
  NO_SERVICE=0
  # Mock systemctl to avoid messing with live system
  systemctl() { return 0; }
  install_systemd_service
)

assert_eq "true" "$([[ -f "$MOCK_HOME/.config/systemd/user/emoji-picker.service" ]] && echo true || echo false)" "Systemd service file should be created"
dropin_content=$(cat "$MOCK_HOME/.config/systemd/user/emoji-picker.service.d/display.conf")
assert_contains "Environment=DISPLAY=:42" "$dropin_content" "display.conf should have Environment=DISPLAY=:42"
assert_contains "Environment=XAUTHORITY=$MOCK_HOME/.test_xauth" "$dropin_content" "display.conf should have mock XAUTHORITY"

# 8. Test Uninstall logic in mock environment
echo "[Test 8] Uninstall mode"
(
  export XDG_CONFIG_HOME="$MOCK_HOME/.config"
  export HOME="$MOCK_HOME"
  BINDIR="$MOCK_HOME/.local/bin"
  SYSTEMD_USER_DIR="$MOCK_HOME/.config/systemd/user"
  SERVICE_DROPIN_DIR="$SYSTEMD_USER_DIR/emoji-picker.service.d"
  mkdir -p "$BINDIR"
  touch "$BINDIR/emoji-picker"
  # shellcheck source=/dev/null
  source "$INSTALLER" --no-service --no-build --dry-run >/dev/null 2>&1 || true
  DRY_RUN=0
  systemctl() { return 0; }
  do_uninstall
)

assert_eq "false" "$([[ -f "$MOCK_HOME/.local/bin/emoji-picker" ]] && echo true || echo false)" "Binary should be removed on uninstall"
assert_eq "false" "$([[ -f "$MOCK_HOME/.config/systemd/user/emoji-picker.service" ]] && echo true || echo false)" "Service file should be removed on uninstall"

# 9. Test Binary --version & --help flags
echo "[Test 9] Binary --version and --help flags"
if [[ -x "$ROOT_DIR/emoji-picker" ]]; then
  ver_out=$("$ROOT_DIR/emoji-picker" --version)
  assert_contains "emoji-picker" "$ver_out" "--version should output emoji-picker"
  help_out=$("$ROOT_DIR/emoji-picker" --help)
  assert_contains "Usage: emoji-picker" "$help_out" "--help should output usage"
fi

# 10. Test Update check logic with mock versions
echo "[Test 10] Update check logic (up-to-date vs update available)"
(
  # shellcheck source=/dev/null
  source "$INSTALLER" --dry-run >/dev/null 2>&1 || true
  # Test version comparison
  assert_eq "0" "$(version_gt "0.3.0" "0.2.0"; echo $?)" "version_gt 0.3.0 > 0.2.0 should return 0 (true)"
  assert_eq "1" "$(version_gt "0.2.0" "0.2.0"; echo $?)" "version_gt 0.2.0 > 0.2.0 should return 1 (false)"
  assert_eq "1" "$(version_gt "0.1.9" "0.2.0"; echo $?)" "version_gt 0.1.9 > 0.2.0 should return 1 (false)"
)

# Test check-update when newer version is available
MOCK_UPDATE_OUT=$(MOCK_LATEST_VERSION="v9.9.9" "$INSTALLER" --check-update)
assert_contains "An update is available" "$MOCK_UPDATE_OUT" "check-update should report available update when newer version detected"

# 11. Test Update in dry-run mode
echo "[Test 11] Update dry-run execution"
UPDATE_DRY_RUN_OUT=$("$INSTALLER" --update --dry-run)
assert_contains "Updating emoji-picker" "$UPDATE_DRY_RUN_OUT" "--update should initiate update process"
assert_contains "[dry-run]" "$UPDATE_DRY_RUN_OUT" "--update --dry-run should indicate dry-run steps"

echo ""
echo "=== Test Results: $PASS_COUNT passed, $FAIL_COUNT failed ==="
if [[ $FAIL_COUNT -gt 0 ]]; then
  exit 1
fi
exit 0

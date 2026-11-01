#!/usr/bin/env bash
# Smoke: keep_above picker stays up; windowfocus + Ctrl+V; clear CLIPBOARD.
set -euo pipefail
RESULT=/tmp/emoji-noblink-result.txt
rm -f "$RESULT"

python3 - <<'PY' &
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib
picker = Gtk.Window(title='FakeEmojiPicker')
picker.set_keep_above(True)
picker.set_accept_focus(False)
picker.set_default_size(200, 120)
picker.add(Gtk.Label(label='picker visible'))
picker.move(20, 20)
entry_win = Gtk.Window(title='EmojiInsertTarget')
entry = Gtk.Entry()
entry_win.add(entry)
entry_win.set_default_size(360, 80)
entry_win.move(280, 40)
def dump(*_):
    open('/tmp/emoji-noblink-result.txt', 'w').write(entry.get_text())
    Gtk.main_quit()
    return False
GLib.timeout_add(3500, dump)
picker.show_all()
entry_win.show_all()
entry.grab_focus()
print('ready', flush=True)
Gtk.main()
PY

for _ in $(seq 1 40); do
  xdotool search --name '^EmojiInsertTarget$' >/dev/null 2>&1 && break
  sleep 0.1
done
target=$(xdotool search --name '^EmojiInsertTarget$' | tail -1)
picker=$(xdotool search --name '^FakeEmojiPicker$' | tail -1)
test -n "$target" && test -n "$picker"

# Keep picker above; only focus target (no activate / no keep_above toggle)
xdotool windowactivate "$picker" || true
sleep 0.05
printf '🐸' | xclip -selection clipboard -t UTF8_STRING
sleep 0.05
xdotool windowfocus "$target"
sleep 0.10
xdotool key --clearmodifiers ctrl+v
sleep 0.25
printf '' | xclip -selection clipboard -t UTF8_STRING -i
sleep 0.1
clip=$(timeout 1 xclip -selection clipboard -o 2>/dev/null || true)
wait || true
got=$(cat "$RESULT" 2>/dev/null || true)
echo "entry=$got clip=$(printf '%s' "$clip" | xxd -p)"
[[ "$got" == *🐸* && -z "$clip" ]] || { echo FAIL; exit 1; }
echo PASS

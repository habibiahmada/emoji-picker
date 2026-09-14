# Shortcuts

Primary binding: **Super+Period** (Win+.).

> [!TIP]
> The `./install.sh` script automatically configures and reloads this shortcut for Openbox, LXQt, GNOME, XFCE, and i3/Sway. This document provides manual reference details.

That key should run:

```bash
~/.local/bin/emoji-picker --toggle
```

The daemon must already be running (`emoji-picker.service`).

On this Lubuntu/LXQt + Openbox setup, bind **both** layers — history shows that
Super shortcuts sometimes need both to stick.

## LXQt (settings file)

File: `~/.config/lxqt/globalkeyshortcuts.conf`

Example (section number must be unique):

```ini
[Meta%2Bperiod.60]
Comment=Emoji picker
Enabled=true
Exec=/home/USER/.local/bin/emoji-picker, --toggle
```

Reload:

```bash
killall lxqt-globalkeysd 2>/dev/null; lxqt-globalkeysd &
```

**Preferred GUI path:** Menu → Preferences → LXQt Settings → Shortcut Keys → Add
Super+Period → same command. Use the GUI when file edits and the live config diverge.

## Openbox

File: `~/.config/openbox/rc.xml` inside `<keyboard>`:

```xml
<keybind key="W-period">
  <action name="Execute">
    <command>/home/USER/.local/bin/emoji-picker --toggle</command>
  </action>
</keybind>
```

```bash
openbox --reconfigure
```

Replace `USER` with your username.

## Conflict history

Emote Flatpak was tried for Win+. and later removed (slow/heavy). Do not rebind
Win+. to Flatpak Emote or `gnome-characters` unless you ask for that on purpose.

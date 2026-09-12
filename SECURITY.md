# Security Policy

## Supported versions

Security fixes are applied to the latest `main` branch. There are no long-term
support releases yet — please upgrade to the newest commit when a fix lands.

## Reporting a vulnerability

Please **do not** open a public GitHub issue for security problems.

Prefer one of these private channels:

1. **GitHub Security Advisories** (preferred):  
   https://github.com/habibiahmada/emoji-picker/security/advisories/new
2. Email the maintainer: **habibiahmadaziz@gmail.com**  
   Use a clear subject such as `[SECURITY] emoji-picker …`.

Include, when possible:

- Affected commit / version / install path
- Desktop environment (e.g. LXQt + Openbox) and whether you are on **X11**
- Steps to reproduce
- Impact (e.g. unexpected code execution via IPC, clipboard/type injection)

You should receive an acknowledgement within a few days. Please give us
reasonable time to ship a fix before any public disclosure.

## Scope notes

This project is a local X11 helper (GTK3 + Unix socket + `xdotool`). Reports
that are especially useful:

- Abuse of `$XDG_RUNTIME_DIR/emoji-picker.sock` by other local users
- Unexpected focus / typing into the wrong window
- Path or command injection via generated data / scripts

Out of scope (unless they enable privilege escalation beyond the user session):

- “Emoji can be typed into a password field if that field is focused”
- Cosmetic UI issues
- Distro packaging CVEs in GTK/`xdotool` themselves (report upstream)

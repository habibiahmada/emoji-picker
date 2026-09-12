# Change checklists

## Insert / paste

- [ ] Defer insert; **ungrab**; keep_above stays on (no blink / no activate)
- [ ] `windowfocus` + Ctrl+V; clear CLIPBOARD ~450ms later
- [ ] Brave: emoji appears, no freeze, no visual blink, clipboard empty after
- [ ] If paste fails without raise: revisit; if Brave freezes: brief hide fallback
- [ ] If Brave was already frozen: restart once, then retest

## Emoji set

- [ ] `make data` from Unicode `latest` (or pinned `UNICODE_VER`)
- [ ] Rebuild + restart user service
- [ ] Spot-check search for a new emoji name

## Shortcuts

- [ ] Update `docs/SHORTCUTS.md`
- [ ] Touch Openbox `rc.xml` **and** LXQt `globalkeyshortcuts.conf` (or tell user to use GUI)
- [ ] `openbox --reconfigure` and restart `lxqt-globalkeysd`

## Agent docs

- [ ] Edit `.agent/` first (CONTEXT / RULES / CHECKLISTS)
- [ ] Stubs still point here (`AGENTS.md`, `CLAUDE.md`, `.cursor/rules/`, …)
- [ ] If Graphify graph is stale after large refactors: `graphify update .`

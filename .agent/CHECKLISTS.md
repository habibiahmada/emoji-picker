# Change checklists

## Insert / paste

- [ ] Still targets previous X11 window id
- [ ] Works while picker stays visible
- [ ] Does not require hiding the popup

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

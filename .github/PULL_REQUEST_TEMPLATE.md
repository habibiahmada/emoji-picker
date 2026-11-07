## Summary

<!-- What and why (not only what). -->

## Test plan

- [ ] `make && make test` (`make data` first if emoji data or keywords changed)
- [ ] `make install` + `systemctl --user restart emoji-picker.service` (if install path)
- [ ] `emoji-picker --toggle` opens near the pointer
- [ ] Insert goes to the **pre-picker** window; picker stays open for multi-pick
- [ ] Esc / click-outside hides; daemon keeps running
- [ ] Docs updated if shortcuts or UX changed (`docs/SHORTCUTS.md`, README)

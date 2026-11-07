# Development

Friendly notes for hacking on the picker. Agent rules live in [`.agent/`](../.agent/).

## Rebuild after Unicode updates

```bash
make data
make
make install
systemctl --user restart emoji-picker.service
```

`src/generated/` is committed so clones can build offline. Refresh it when you
ship emoji updates. `make data` downloads Unicode `emoji-test.txt` and CLDR
keyword files into `data/` (gitignored); only `data/keywords-extra.txt` is
tracked.

## Tests

```bash
make test    # unit tests (search, history, model, skin tone, grid) + installer tests
```

## Search keywords

A word people use does not find an emoji? Add it to `data/keywords-extra.txt`
(`<emoji> ; word word …`), run `make data && make`, and check it with a search.

## Debug without systemd

```bash
systemctl --user stop emoji-picker.service
./emoji-picker          # shows immediately
# other terminal:
./emoji-picker --toggle
```

Follow logs:

```bash
journalctl --user -u emoji-picker.service -f
```

## Common failures

| What you see | What to check |
|--------------|----------------|
| `--toggle` does nothing | Socket missing → daemon not running |
| Window never appears | `DISPLAY` / `XAUTHORITY` in the service drop-in |
| Emoji are empty boxes | Install `fonts-noto-color-emoji` |
| A newer emoji is missing | Font older than its Unicode version; it is hidden on purpose |
| Paste lands in the search box | Target xid not set / `xdotool` missing |
| Win+. does nothing | LXQt **and** Openbox unbound, or daemon down |

## Scripts line endings

Must be Unix LF. If you see `env: 'bash\r'`:

```bash
sed -i 's/\r$//' scripts/*
```

## Screenshots

See [SCREENSHOTS.md](SCREENSHOTS.md). Assets live in `docs/screenshots/`.

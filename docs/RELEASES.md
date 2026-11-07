# Releases

Distribution channel: **GitHub Releases** only (no Flatpak/apt yet).

## For users

```bash
curl -fsSL https://raw.githubusercontent.com/habibiahmada/emoji-picker/main/get.sh | sh
```

`get.sh` downloads `emoji-picker-VERSION-linux-x86_64.tar.gz` plus its `.sha256`
from the latest release, verifies it and runs the bundled `install.sh`. You can
also download the tarball from
https://github.com/habibiahmada/emoji-picker/releases and follow `INSTALL.txt`.

## For maintainers

1. Bump the version in `Makefile` (`VERSION`), `src/main.c` (`APP_VERSION`) and
   the fallback in `install.sh`; update the "What's New" block in
   `scripts/release.sh` and the version examples in `README.md`.
2. Merge to `main`, then push an annotated tag. CI (`.github/workflows/release.yml`,
   on `v*` tags) runs the tests, builds the tarball and attaches it to the release:

```bash
git tag -a v0.5.0 -F release-notes.md
git push origin v0.5.0
```

Build the assets locally without publishing: `make dist VERSION=0.5.0`.
`./scripts/release.sh 0.5.0 --publish` tags and publishes from your machine instead.

The tarball and checksum names are what `get.sh` downloads; keep them in sync.

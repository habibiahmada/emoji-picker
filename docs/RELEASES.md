# Releases

Distribution channel: **GitHub Releases** only (no Flatpak/apt yet).

## For users

Download `emoji-picker-VERSION-linux-x86_64.tar.gz` from
https://github.com/habibiahmada/emoji-picker/releases and follow `INSTALL.txt`.

## For maintainers

```bash
# Build assets only
make dist VERSION=0.1.0

# Tag + publish (needs gh auth)
./scripts/release.sh 0.1.0 --publish
```

Or push an annotated tag and let CI publish:

```bash
git tag -a v0.1.0 -m "Release v0.1.0"
git push origin v0.1.0
```

CI workflow: `.github/workflows/release.yml` (runs on `v*` tags).

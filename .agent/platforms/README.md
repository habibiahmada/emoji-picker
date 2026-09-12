# Platform wiring

Canonical content: parent folder `.agent/`.

Each assistant discovers instructions differently. This repo keeps **thin stubs**
that point here so you never maintain five copies of the same rules.

| Tool | Stub / discovery path | Notes |
|------|----------------------|--------|
| [Cursor](cursor.md) | `.cursor/rules/*.mdc`, root `AGENTS.md` | Rules always applied via `.mdc` |
| [Antigravity](antigravity.md) | `.agent/rules/*.md`, root `AGENTS.md` | Prefers `.agents/rules`; `.agent/rules` still supported |
| [Kiro](kiro.md) | `.kiro/steering/*.md`, root `AGENTS.md` | Steering + AGENTS.md always-on |
| [GitHub Copilot](copilot.md) | `.github/copilot-instructions.md` | Plus optional `.github/instructions/` |
| [Claude Code](claude.md) | `CLAUDE.md` | Optional `.claude/rules/` |
| Graphify | `.agent/GRAPHIFY.md` | Skill via `graphify install --platform …` |

When editing guidance: change `.agent/CONTEXT.md` / `RULES.md` first, then glance
at stubs only if a tool needs a unique frontmatter tweak.

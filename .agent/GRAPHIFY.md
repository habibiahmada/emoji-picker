# Graphify — code knowledge graph

[Graphify](https://graphify.com/docs) maps this repo into a local, queryable graph
so agents can walk calls/imports instead of grepping blindly.

## Install (once per machine)

```bash
uv tool install graphifyy
# optional MCP tools:
# uv tool install "graphifyy[mcp]"
```

Copy the skill into assistants you use:

```bash
graphify install --platform cursor
graphify install --platform antigravity
graphify install --platform kiro
graphify install --platform agents   # AGENTS.md-compatible assistants
# Claude Code / Copilot: see https://graphify.com/integrations
```

Only run install commands when the user asks — they touch host config dirs.

## Build / refresh the graph (in this repo)

```bash
cd /home/habibiahmada/Projects/emoji-picker
graphify update .                 # code AST only (no LLM)
# or in chat: /graphify
```

Outputs (gitignored):

| Path | Meaning |
|------|---------|
| `graphify-out/graph.json` | Machine graph |
| `graphify-out/GRAPH_REPORT.md` | Human architecture report (after full `/graphify`) |
| `graphify-out/graph.html` | Interactive view (when generated) |

## Useful queries for agents

```bash
graphify query "where is emoji paste handled?"
graphify affected "emoji_insert"
graphify path "emoji_popup_show" "emoji_insert"
graphify god-nodes --top 10
graphify explain "popup.c"
```

Prefer these before large exploratory greps when changing insert, IPC, or popup flow.

## Project convention

- Treat `graphify-out/` as **generated** — do not hand-edit.
- After big refactors that delete symbols: `graphify update . --force` if the tool refuses a smaller rebuild.
- Document Graphify changes in `.agent/` only; do not scatter install notes into every platform stub.

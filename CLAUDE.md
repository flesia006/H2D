# CLAUDE.md

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:

```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Then loop until each verification passes.

---

# Project: Top-Down Hybrid (PaperZD)

## What this is

An Unreal Engine, Blueprint-driven top-down game using the **PaperZD** plugin for
2D sprite-based character animation. Characters include Mannequin, Jemie, and X
(2D paper sprites with multi-direction animation sets). Includes a HUD widget.

- Remote: `github.com/flesia006/H2D.git` (branch `main`)
- Most logic lives in Blueprints/assets (`.uasset`), not C++.

## Environment

- Windows 11, shell is **PowerShell** (use PowerShell syntax, not bash, unless using the Bash tool).
- An **Unreal Editor MCP** is available (`mcp__unreal-editor__execute_script`) for
  running scripts inside the live UE editor — prefer it for editor-side inspection/automation.

## Conventions

- Asset/character naming follows existing patterns: `BP_*` (blueprints), `GM_*`
  (game modes), `L_*` (levels/maps), `M_*` (materials), `AS_*` (PaperZD anim source).
  Match these when adding assets.
- Binaries, `Saved/`, `Intermediate/`, `DerivedDataCache/`, `*_BuiltData.uasset`,
  and build output are git-ignored (see `.gitignore`).

## Daily work-log automation (`.devtools/daily-report/`)

A `git push` triggers a `pre-push` hook that auto-generates a Korean daily work log
and posts it to Notion. Flow:

1. `.git/hooks/pre-push` computes the pushed commit range and launches the script detached (never blocks the push).
2. `daily-report.mjs` collects commits/diff → summarizes via the `claude` CLI (`-p`) → saves an HTML copy → creates a Notion page.
3. Output format is governed by **`Notion.md`** (the `## 작성 규칙` section is injected into the prompt) — edit that file to change the log format, not the code.

Notes for working in this folder:
- `config.json` holds the Notion token + parent ID and is **git-ignored** — never commit it or print its contents.
- The whole `.devtools/` folder is git-ignored to keep the changelist clean and secrets out of git.
- Logs are in `.devtools/daily-report/daily-report.log`; generated reports in `.devtools/daily-report/reports/`.

## Communication

- The user prefers Korean for discussion.

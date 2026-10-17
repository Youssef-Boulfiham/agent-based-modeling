# Finance agent: Geavanceerde informatie

Branch `finance-agent` of the ABM project. It is a **standalone project with its own
history** (an orphan branch, never merged into `main`). It holds the dashboard behind
the single ABM agent that manages my personal finances: click the agent, then
**Geavanceerde informatie**, and this page opens. The ABM side of that link is not
built yet; see [Connecting to the ABM app](#connecting-to-the-abm-app).

## Open it

- Double-click `index.html`, or run `open index.html`.
  It is one self-contained file: no server, no install, no internet.
- To see it with example data, open `index.html?demo`. Demo mode never saves anything.

## What it does

| Part | What you get |
|------|--------------|
| **Toevoegen** | Log an expense in a few seconds: amount, optional description, date and category, then Enter. A description you have used before selects its usual category. **Vaak gebruikt** re-adds a frequent expense in one click. |
| **Overzicht** | Spent this month, with a budget meter and a marker for where an even month would be today. Also today, this week vs last week, the daily average and the expected month total. |
| **Agent-inzichten** | Up to three plain-language notes: budget pace, the category that changed most vs the same days last month, the biggest category, days without spending. |
| **Over tijd** | 7 days / 30 days (per day), 3 months (per week), 12 months / all (per month). Includes a chart with a table view, a per-category breakdown (click a category to filter) and a searchable day-by-day history. Click a row to edit or delete it (with undo). |
| **Instellingen** | Monthly budget, categories, backup and restore (JSON), CSV export for Excel/Numbers, and wiping all data. |

Averages only count days since you started tracking, so the first weeks are not
diluted. Comparisons with last week or last month only appear once that period
was fully tracked.

## Your data

- Data is stored **only in this browser on this device** (`localStorage`, key
  `abm.finance.v1`) and never leaves your computer.
- Each browser keeps its own copy, so Safari and Chrome do not share data.
  Clearing website data deletes it. Download a backup now and then via
  **Instellingen → Backup downloaden**. The footer warns when the last backup is
  more than 30 days old.
- **Never commit a backup or CSV export: this repository is public.**
  `.gitignore` blocks the default file names (`financien-*.json`, `financien-*.csv`).

### Data format (v1)

A backup file has exactly this format. It is also the contract for the ABM agent
if it reads or writes the data later.

```json
{
  "version": 1,
  "settings": { "budget": 120000 },
  "categories": [
    { "id": "boodschappen", "name": "Boodschappen" },
    { "id": "overig", "name": "Overig" }
  ],
  "expenses": [
    {
      "id": "lq3k9x2a7f1c",
      "date": "2026-10-17",
      "amount": 450,
      "category": "eten-drinken",
      "note": "Koffie",
      "created": "2026-10-17T08:12:00.000Z"
    }
  ],
  "meta": { "created": "2026-10-01T09:00:00.000Z", "lastBackup": null }
}
```

- `amount` and `budget` are **integer cents** (`450` = € 4,50; `0` = no budget).
- `date` is the local calendar day `YYYY-MM-DD`. Future dates are refused.
- The category `overig` always exists. Expenses of a deleted category move there.
- On import, invalid rows are dropped and unknown categories become `overig`.

## Connecting to the ABM app

This is a proposal. It is not implemented in `main`, because the ABM workflow
(`CLAUDE.md` on `main`) requires approval and a testcase before anything goes
into `src/`.

1. **Put this branch next to the ABM code.** From the ABM project root:
   ```bash
   git fetch origin finance-agent
   git worktree add agents/finance finance-agent
   ```
   `agents/finance/index.html` now exists on disk, and the branch keeps its own
   history. Add `agents/` to `.gitignore` on `main`.
2. **Add the button in the app.** Give the agent a **Geavanceerde informatie**
   button, either in the Agents settings panel (`SettingsWindow::renderAgents`) or
   in a popup when you click the agent in the env. The button calls
   `SDL_OpenURL("file://<project root>/agents/finance/index.html")`. SDL2 already
   provides this, so no new dependency is needed.

## Files

- `index.html`: the whole app (HTML, CSS and JS, no dependencies).
- `CLAUDE.md`: rules for working on this branch.
- `.gitignore`: keeps personal finance exports out of git.

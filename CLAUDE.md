# Finance agent branch

**Read `README.md` before working here.**

## Branch: MANDATORY

- `finance-agent` is a standalone orphan branch of the ABM repo. **Never merge it into
  `main` and never rebase it onto `main`.** The ABM app links to it; it does not contain it.
- **NEVER commit or push unless the user explicitly says so** (same rule as `main`).
- **Never commit personal data** (backups, CSV exports, screenshots with real numbers).
  The repository is public.

## Product rules

- **Simplicity above all.** Every feature must keep logging an expense a few-second job.
  Prefer removing something over adding a setting.
- **One file:** `index.html` holds all HTML, CSS and JS. No build step, no external
  scripts, fonts or CDNs. It must work offline from `file://` and keep data private.
- UI language is Dutch: `nl-NL` formatting (`€ 1.234,56`), and weeks start on Monday.

## Code rules

- Money is stored as **integer cents**. Format with `money()`/`money0()`, parse with `parseAmount()`.
- Dates are local `YYYY-MM-DD` strings. Use `keyOf()`/`dateOf()`, never
  `new Date('YYYY-MM-DD')`, because that parses as UTC and shifts the day.
- To change the data format, bump `version`, migrate in `normalize()`, and keep old
  backups importable. Update the format section in `README.md`.
- User data reaches the DOM only as text (the `h()` helper / `textContent`), never via `innerHTML`.
- Charts use one data color (`--accent`) from the validated dataviz palette. Status
  colors (good/bad) always come with an icon and text. Every chart has a table view.

## Test before calling it done

- Open `index.html?demo` in light and dark mode, at desktop width and at 390px phone width.
- Check that adding, editing, deleting with undo, settings, backup/restore and CSV
  export all work, and that the console shows no errors.

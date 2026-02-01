# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
make            # Build (g++ -std=c++17 -Wall -Wextra -O2)
make clean      # Remove binary
./dashboard     # Run with default settings
./dashboard -l "New York, NY"   # Custom weather location
./dashboard --no-color          # Disable ANSI colors
```

No test suite or linter is configured. Verify changes by running `make && ./dashboard` and inspecting the output.

## Architecture

Single-file C++ CLI app (`src/main.cpp`, ~760 lines) that aggregates data from four free APIs into a terminal dashboard. No external libraries — all HTTP is done by shelling out to `curl` via `popen()`, and JSON/XML responses are parsed with custom substring-matching functions.

### Execution flow in `main()`

1. **Weather** (`showWeather`) — wttr.in, 10s timeout
2. **Joke** (`showJoke`) — JokeAPI v2, 5s timeout
3. **News** (`showNews`) — Google News RSS, 5s timeout
4. **Sports** (`showSports`) — ESPN scoreboard API, 5s timeout per request

Each section handles its own fetch, parse, and display. Failures degrade gracefully with fallback messages.

### Key components

- **`namespace color`** — ANSI escape codes with a global `enabled` flag; auto-disabled when stdout is not a TTY.
- **`exec(cmd)`** — Runs a shell command and returns stdout as a string. Used by every section.
- **`jsonValue` / `jsonValueFrom`** — Stateless and stateful JSON string extractors (no library). `jsonValueFrom` advances a position reference for sequential field extraction.
- **`xmlTags(xml, tag, limit)`** — Extracts inner text from XML tags with CDATA stripping. Used for RSS parsing.
- **`struct Game`** — Holds away/home teams, scores, status, and recap text.
- **`parseESPNScoreboard(json)`** — Parses ESPN's scoreboard JSON by searching for `"shortName"` markers, then extracting scores, status (`shortDetail`), and headlines. Returns a `vector<Game>`.

### Sports section details

The sports display is the most complex section (~400 lines). Key design points:

- **Dual-date fetch**: Queries both today (T) and yesterday (T-1) scoreboards per league to always show recent completed games.
- **East Coast filter**: Each league has a hardcoded set of team abbreviations in the `eastCoast` map. Only games involving those teams are shown.
- **Three-way split**: Games are categorized into `completedToday`, `completedYesterday`, and `upcoming` (any non-"Final" status).
- **Two-column table**: Completed games render in a Unicode box with scores (40 chars) on the left and word-wrapped recaps (46 chars) on the right. Upcoming games use a simpler full-width single-column layout.
- **Box-drawing transitions**: Border characters change between two-column and single-column sections (e.g., `┴` at the column junction when transitioning to the upcoming section).

### ESPN API pattern

```
https://site.api.espn.com/apis/site/v2/sports/{sport}/{league}/scoreboard?dates=YYYYMMDD
```

No auth required; a `User-Agent` header is sent. The date parameter is formatted with `strftime("%Y%m%d")`.

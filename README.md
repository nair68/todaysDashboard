# todaysDashboard

A C++ command-line tool that displays a quick daily dashboard with today's weather, a joke, top news headlines, and live sports scores. No API keys required.

## Features

- **Weather** -- Current conditions via [wttr.in](https://wttr.in) (auto-detects location or specify one)
- **Joke** -- Random joke from [JokeAPI](https://v2.jokeapi.dev)
- **News** -- Top 3 headlines from [Google News RSS](https://news.google.com/rss)
- **Sports** -- NFL, NBA, NHL, and MLB scores from [ESPN](https://site.api.espn.com) (winning teams highlighted in green)

## Requirements

- C++17 compiler (g++, clang++)
- `curl` (command-line)
- `make`

## Build

```bash
make
```

## Usage

```bash
# Auto-detect location
./dashboard

# Specify a location
./dashboard -l "Denver, Colorado"
./dashboard --location "Miami, FL"

# Disable colored output
./dashboard --no-color

# Help
./dashboard --help
```

## Options

| Flag | Description |
|------|-------------|
| `-l`, `--location "City, State"` | Set weather location (default: auto-detect by IP) |
| `--no-color` | Disable colored terminal output |
| `-h`, `--help` | Show help message |

Colors are enabled by default and auto-disable when output is piped to a file or another command.

## Example Output

```
============================================================
  TODAY'S DASHBOARD  --  Saturday, January 31, 2026
============================================================

  TODAY'S WEATHER
------------------------------------------------------------
  Location:    Park Ridge, New Jersey, United States
  Condition:   Overcast
  Temperature: +22°F
  Humidity:    47%
  Wind:        ↙2mph

------------------------------------------------------------

  JOKE OF THE MOMENT
------------------------------------------------------------
  Why did the koala get rejected?
  ... Because he did not have any koalafication.

------------------------------------------------------------

  TODAY'S HEADLINES
------------------------------------------------------------
  1. Headline one - Source
  2. Headline two - Source
  3. Headline three - Source

------------------------------------------------------------

  SPORTS SCORES
------------------------------------------------------------
  NFL
    NFC 0  @  AFC 0  (2/3 - 8:00 PM EST)

  NBA
    SA 106  @  CHA 111  (Final)
    ATL 124  @  IND 129  (Final)
    CHI 125  @  MIA 118  (Final)

  NHL
    COL 5  @  DET 0  (Final)
    NYR 5  @  PIT 6  (Final)
    TOR 3  @  VAN 2  (Final/SO)

  MLB
    NYY 0  @  BAL 0  (2/20 - 1:05 PM EST)
    SD 0  @  SEA 0  (2/20 - 3:10 PM EST)

============================================================
```

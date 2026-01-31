# todaysDashboard

A C++ command-line tool that displays a quick daily dashboard with today's weather, a joke, and top news headlines. No API keys required.

## Features

- **Weather** -- Current conditions via [wttr.in](https://wttr.in) (auto-detects location or specify one)
- **Joke** -- Random joke from [JokeAPI](https://v2.jokeapi.dev)
- **News** -- Top 3 headlines from [Google News RSS](https://news.google.com/rss)

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

# Help
./dashboard --help
```

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

============================================================
```

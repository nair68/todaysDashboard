#include <iostream>
#include <string>
#include <array>
#include <cstdio>
#include <vector>
#include <ctime>
#include <algorithm>
#include <set>
#include <map>
#include <cstdlib>
#include <unistd.h>

// ANSI color codes
namespace color {
    const char* reset   = "\033[0m";
    const char* bold    = "\033[1m";
    const char* dim     = "\033[2m";
    const char* cyan    = "\033[36m";
    const char* yellow  = "\033[33m";
    const char* green   = "\033[32m";
    const char* magenta = "\033[35m";
    const char* blue    = "\033[34m";
    const char* white   = "\033[37m";
    const char* red     = "\033[31m";

    bool enabled = true;

    const char* c(const char* code) { return enabled ? code : ""; }
}

// URL-encode a string for wttr.in (spaces -> +, preserve commas for readability)
std::string urlEncode(const std::string& str) {
    std::string encoded;
    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == ',') {
            encoded += c;
        } else if (c == ' ') {
            encoded += '+';
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
            encoded += buf;
        }
    }
    return encoded;
}

// Execute a shell command and return its stdout as a string
std::string exec(const std::string& cmd) {
    std::array<char, 4096> buffer;
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return "Error: failed to run command.";
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

// Extract a JSON string value by key (simple parser for flat JSON)
std::string jsonValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) {
        // Try with a space after the colon
        search = "\"" + key + "\": \"";
        pos = json.find(search);
        if (pos == std::string::npos) return "";
    }
    auto start = pos + search.size();
    std::string value;
    for (size_t i = start; i < json.size(); ++i) {
        if (json[i] == '\\' && i + 1 < json.size()) {
            char next = json[i + 1];
            if (next == 'n') value += '\n';
            else if (next == 't') value += '\t';
            else if (next == 'r') value += '\r';
            else value += next;
            ++i;
        } else if (json[i] == '"') {
            break;
        } else {
            value += json[i];
        }
    }
    return value;
}

// Extract a JSON string value by key, searching from a given position
// Returns the value and updates searchFrom to point past the found value
std::string jsonValueFrom(const std::string& json, const std::string& key, size_t& searchFrom) {
    std::string search = "\"" + key + "\":\"";
    auto pos = json.find(search, searchFrom);
    if (pos == std::string::npos) {
        search = "\"" + key + "\": \"";
        pos = json.find(search, searchFrom);
        if (pos == std::string::npos) return "";
    }
    auto start = pos + search.size();
    std::string value;
    size_t i = start;
    for (; i < json.size(); ++i) {
        if (json[i] == '\\' && i + 1 < json.size()) {
            char next = json[i + 1];
            if (next == 'n') value += '\n';
            else if (next == 't') value += '\t';
            else if (next == 'r') value += '\r';
            else value += next;
            ++i;
        } else if (json[i] == '"') {
            break;
        } else {
            value += json[i];
        }
    }
    searchFrom = i;
    return value;
}

// Extract text between XML tags: <tag>...</tag>
std::vector<std::string> xmlTags(const std::string& xml, const std::string& tag, int limit) {
    std::vector<std::string> results;
    std::string openTag = "<" + tag + ">";
    std::string closeTag = "</" + tag + ">";
    size_t pos = 0;
    while (results.size() < static_cast<size_t>(limit)) {
        pos = xml.find(openTag, pos);
        if (pos == std::string::npos) break;
        auto start = pos + openTag.size();
        auto end = xml.find(closeTag, start);
        if (end == std::string::npos) break;
        std::string content = xml.substr(start, end - start);
        // Strip CDATA if present
        if (content.find("<![CDATA[") == 0) {
            content = content.substr(9, content.size() - 12);
        }
        results.push_back(content);
        pos = end + closeTag.size();
    }
    return results;
}

std::string repeatStr(const std::string& s, int n) {
    std::string result;
    for (int i = 0; i < n; ++i) result += s;
    return result;
}

void printSeparator() {
    std::cout << "\n" << color::c(color::dim) << std::string(60, '-')
              << color::c(color::reset) << "\n\n";
}

void showWeather(const std::string& location) {
    std::cout << color::c(color::bold) << color::c(color::yellow)
              << "  TODAY'S WEATHER" << color::c(color::reset) << "\n";
    std::cout << color::c(color::dim) << std::string(60, '-')
              << color::c(color::reset) << "\n";

    // Build wttr.in URL -- if location is provided, include it in the path
    std::string url = "wttr.in/";
    if (!location.empty()) {
        url += urlEncode(location);
    }
    url += "?format=%l\\n%C\\n%t\\n%h\\n%w";

    std::string data = exec(
        "curl -s --max-time 5 \"" + url + "\""
    );

    if (data.empty() || data.find("Unknown") != std::string::npos) {
        std::cout << "  Could not retrieve weather data.\n";
        return;
    }

    // Parse the 5 lines
    std::vector<std::string> lines;
    size_t pos = 0;
    while (pos < data.size()) {
        auto nl = data.find('\n', pos);
        if (nl == std::string::npos) {
            lines.push_back(data.substr(pos));
            break;
        }
        lines.push_back(data.substr(pos, nl - pos));
        pos = nl + 1;
    }

    if (lines.size() >= 5) {
        // Clean up location display (wttr.in echoes '+' for spaces)
        std::string loc = lines[0];
        std::replace(loc.begin(), loc.end(), '+', ' ');
        std::cout << color::c(color::bold) << "  Location:    " << color::c(color::reset) << color::c(color::green) << loc << color::c(color::reset) << "\n";
        std::cout << color::c(color::bold) << "  Condition:   " << color::c(color::reset) << color::c(color::green) << lines[1] << color::c(color::reset) << "\n";
        std::cout << color::c(color::bold) << "  Temperature: " << color::c(color::reset) << color::c(color::green) << lines[2] << color::c(color::reset) << "\n";
        std::cout << color::c(color::bold) << "  Humidity:    " << color::c(color::reset) << color::c(color::green) << lines[3] << color::c(color::reset) << "\n";
        std::cout << color::c(color::bold) << "  Wind:        " << color::c(color::reset) << color::c(color::green) << lines[4] << color::c(color::reset) << "\n";
    } else {
        // Fallback: just print raw data
        std::cout << "  " << data << "\n";
    }

    std::cout << "\n" << color::c(color::dim) << "  More: https://weather.com" << color::c(color::reset) << "\n";
}

void showJoke() {
    std::cout << color::c(color::bold) << color::c(color::yellow)
              << "  JOKE OF THE MOMENT" << color::c(color::reset) << "\n";
    std::cout << color::c(color::dim) << std::string(60, '-')
              << color::c(color::reset) << "\n";

    std::string json = exec(
        "curl -s --max-time 5 "
        "\"https://v2.jokeapi.dev/joke/Programming,Miscellaneous,Pun"
        "?blacklistFlags=nsfw,religious,political,racist,sexist,explicit\""
    );

    if (json.empty()) {
        std::cout << "  Could not retrieve a joke.\n";
        return;
    }

    std::string type = jsonValue(json, "type");

    if (type == "twopart") {
        std::string setup = jsonValue(json, "setup");
        std::string delivery = jsonValue(json, "delivery");
        std::cout << color::c(color::magenta) << "  " << setup << color::c(color::reset) << "\n";
        std::cout << color::c(color::bold) << color::c(color::magenta) << "  ... " << delivery << color::c(color::reset) << "\n";
    } else if (type == "single") {
        std::string joke = jsonValue(json, "joke");
        std::cout << color::c(color::magenta) << "  " << joke << color::c(color::reset) << "\n";
    } else {
        std::cout << "  Could not parse joke.\n";
    }
}

void showNews() {
    std::cout << color::c(color::bold) << color::c(color::yellow)
              << "  TODAY'S HEADLINES" << color::c(color::reset) << "\n";
    std::cout << color::c(color::dim) << std::string(60, '-')
              << color::c(color::reset) << "\n";

    std::string rss = exec(
        "curl -s --max-time 5 "
        "\"https://news.google.com/rss?hl=en-US&gl=US&ceid=US:en\""
    );

    if (rss.empty()) {
        std::cout << "  Could not retrieve news.\n";
        return;
    }

    // The first <title> is the feed title ("Google News"), skip it.
    // Item titles come inside <item><title>...</title></item>.
    // Extract titles from <item> blocks.
    std::vector<std::string> items;
    size_t pos = 0;
    int count = 0;
    while (count < 3) {
        pos = rss.find("<item>", pos);
        if (pos == std::string::npos) break;
        auto itemEnd = rss.find("</item>", pos);
        if (itemEnd == std::string::npos) break;
        std::string item = rss.substr(pos, itemEnd - pos);
        auto titles = xmlTags(item, "title", 1);
        if (!titles.empty()) {
            items.push_back(titles[0]);
            ++count;
        }
        pos = itemEnd;
    }

    if (items.empty()) {
        std::cout << "  Could not parse news headlines.\n";
        return;
    }

    for (size_t i = 0; i < items.size(); ++i) {
        std::cout << color::c(color::cyan) << "  " << (i + 1) << ". "
                  << color::c(color::reset) << color::c(color::white)
                  << items[i] << color::c(color::reset) << "\n";
    }

    std::cout << "\n" << color::c(color::dim) << "  More: https://news.google.com" << color::c(color::reset) << "\n";
}

struct Game {
    std::string away;
    std::string home;
    std::string awayScore;
    std::string homeScore;
    std::string status;
    std::string recap;
};

std::vector<Game> parseESPNScoreboard(const std::string& json) {
    std::vector<Game> games;
    std::string marker = "\"shortName\":\"";
    size_t pos = 0;

    while (true) {
        pos = json.find(marker, pos);
        if (pos == std::string::npos) break;
        auto start = pos + marker.size();
        auto end = json.find('"', start);
        if (end == std::string::npos) break;
        std::string shortName = json.substr(start, end - start);

        // Parse "AWAY @ HOME" or "AWAY VS HOME" from shortName
        std::string sep = " @ ";
        auto atPos = shortName.find(sep);
        if (atPos == std::string::npos) {
            sep = " VS ";
            atPos = shortName.find(sep);
            if (atPos == std::string::npos) { pos = end; continue; }
        }
        std::string away = shortName.substr(0, atPos);
        std::string home = shortName.substr(atPos + sep.size());

        // Find the two scores (home competitor listed first, then away)
        size_t scoreSearch = end;
        std::string homeScore = jsonValueFrom(json, "score", scoreSearch);
        std::string awayScore = jsonValueFrom(json, "score", scoreSearch);

        // Find game status
        size_t statusSearch = end;
        std::string status = jsonValueFrom(json, "shortDetail", statusSearch);

        // Find headline/recap description for this game
        std::string recap;
        size_t headlinePos = json.find("\"headlines\"", end);
        // Find the next *game* shortName (one containing " @ " or " VS ")
        size_t nextGame = std::string::npos;
        {
            size_t search = end;
            while (true) {
                search = json.find(marker, search);
                if (search == std::string::npos) break;
                auto ns = search + marker.size();
                auto ne = json.find('"', ns);
                if (ne == std::string::npos) break;
                std::string val = json.substr(ns, ne - ns);
                if (val.find(" @ ") != std::string::npos || val.find(" VS ") != std::string::npos) {
                    nextGame = search;
                    break;
                }
                search = ne;
            }
        }
        if (headlinePos != std::string::npos &&
            (nextGame == std::string::npos || headlinePos < nextGame)) {
            size_t descSearch = headlinePos;
            recap = jsonValueFrom(json, "description", descSearch);
            // Strip leading em-dash and whitespace from ESPN recaps
            while (!recap.empty() && (recap[0] == ' ' || recap[0] == '-')) {
                recap.erase(0, 1);
            }
            // Handle UTF-8 em-dash (U+2014: 0xE2 0x80 0x94)
            if (recap.size() >= 3 &&
                static_cast<unsigned char>(recap[0]) == 0xE2 &&
                static_cast<unsigned char>(recap[1]) == 0x80 &&
                static_cast<unsigned char>(recap[2]) == 0x94) {
                recap.erase(0, 3);
                while (!recap.empty() && recap[0] == ' ') recap.erase(0, 1);
            }
            // Truncate to first two sentences or ~200 chars
            if (!recap.empty()) {
                // Find the end of the second sentence
                size_t first = recap.find(". ");
                size_t second = std::string::npos;
                if (first != std::string::npos && first + 2 < recap.size()) {
                    second = recap.find(". ", first + 2);
                }
                if (second != std::string::npos && second < 200) {
                    recap = recap.substr(0, second + 1);
                } else if (first != std::string::npos && first < 200) {
                    recap = recap.substr(0, first + 1);
                } else if (recap.size() > 200) {
                    recap = recap.substr(0, 200) + "...";
                }
            }
        }

        games.push_back({away, home, awayScore, homeScore, status, recap});
        pos = end;
    }
    return games;
}

void showSports() {
    std::cout << color::c(color::bold) << color::c(color::yellow)
              << "  SPORTS SCORES" << color::c(color::reset) << "\n";
    std::cout << color::c(color::dim) << std::string(60, '-')
              << color::c(color::reset) << "\n";

    struct League {
        std::string name;
        std::string url;
    };

    std::vector<League> leagues = {
        {"NFL", "https://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard"},
        {"NBA", "https://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard"},
        {"NHL", "https://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard"},
        {"MLB", "https://site.api.espn.com/apis/site/v2/sports/baseball/mlb/scoreboard"},
    };

    // East Coast teams by league
    std::map<std::string, std::set<std::string>> eastCoast = {
        {"NFL", {"NE", "NYJ", "NYG", "BUF", "MIA", "PHI", "PIT", "BAL", "WAS", "CAR", "ATL", "TB", "JAX"}},
        {"NBA", {"BOS", "BKN", "NY", "PHI", "WAS", "CHA", "ATL", "MIA", "ORL"}},
        {"NHL", {"BOS", "NYR", "NYI", "NJ", "PHI", "PIT", "WAS", "CAR", "FLA", "TB", "BUF"}},
        {"MLB", {"NYY", "NYM", "BOS", "BAL", "TB", "PHI", "WAS", "MIA", "ATL", "PIT"}},
    };

    bool anyGames = false;

    for (const auto& league : leagues) {
        std::string json = exec(
            "curl -s --max-time 5 -H \"User-Agent: Mozilla/5.0\" \"" + league.url + "\""
        );

        if (json.empty()) continue;

        auto games = parseESPNScoreboard(json);
        if (games.empty()) continue;

        // Filter to East Coast games
        const auto& teams = eastCoast[league.name];
        std::vector<Game> filtered;
        for (const auto& g : games) {
            if (teams.count(g.away) || teams.count(g.home)) {
                filtered.push_back(g);
            }
        }
        if (filtered.empty()) continue;

        anyGames = true;

        const int scoreCol = 40;
        const int recapCol = 46;
        const int fullWidth = scoreCol + 1 + recapCol; // +1 for middle border │

        std::string hl = "\u2500"; // ─

        // Top border (full-width, no middle junction)
        std::cout << color::c(color::dim) << "  \u250C"
                  << repeatStr(hl, fullWidth) << "\u2510"
                  << color::c(color::reset) << "\n";
        // League name row (spans full width)
        {
            std::string label = " " + league.name;
            int pad = fullWidth - static_cast<int>(label.size());
            if (pad < 0) pad = 0;
            std::cout << color::c(color::dim) << "  \u2502" << color::c(color::reset)
                      << color::c(color::bold) << color::c(color::blue)
                      << label << std::string(pad, ' ')
                      << color::c(color::reset)
                      << color::c(color::dim) << "\u2502" << color::c(color::reset) << "\n";
        }
        // Divider with column split
        std::cout << color::c(color::dim) << "  \u251C"
                  << repeatStr(hl, scoreCol) << "\u252C"
                  << repeatStr(hl, recapCol) << "\u2524"
                  << color::c(color::reset) << "\n";

        for (const auto& g : filtered) {
            // Build visible score text to measure width for padding
            std::string scoreText = " " + g.away + " " + g.awayScore
                                  + "  @  " + g.home + " " + g.homeScore
                                  + "  (" + g.status + ")";
            int scorePad = scoreCol - static_cast<int>(scoreText.size());
            if (scorePad < 0) scorePad = 0;
            // If score overflows, truncate the status
            if (scorePad == 0 && static_cast<int>(scoreText.size()) > scoreCol) {
                int over = static_cast<int>(scoreText.size()) - scoreCol;
                std::string truncStatus = g.status;
                if (static_cast<int>(truncStatus.size()) > over + 3) {
                    truncStatus = truncStatus.substr(0, truncStatus.size() - over - 3) + "...";
                }
                scoreText = " " + g.away + " " + g.awayScore
                          + "  @  " + g.home + " " + g.homeScore
                          + "  (" + truncStatus + ")";
                scorePad = scoreCol - static_cast<int>(scoreText.size());
                if (scorePad < 0) scorePad = 0;
            }

            // Highlight the winning team
            const char* awayStyle = color::c(color::white);
            const char* homeStyle = color::c(color::white);
            if (g.status.find("Final") != std::string::npos) {
                int as = std::atoi(g.awayScore.c_str());
                int hs = std::atoi(g.homeScore.c_str());
                if (as > hs) awayStyle = color::c(color::green);
                else if (hs > as) homeStyle = color::c(color::green);
            }

            // Word-wrap recap into lines that fit the column (max 3 lines)
            int maxRecap = recapCol - 2; // 1 char padding each side
            std::vector<std::string> recapLines;
            {
                std::string remaining = g.recap;
                int maxLines = 3;
                while (!remaining.empty() && static_cast<int>(recapLines.size()) < maxLines) {
                    if (static_cast<int>(remaining.size()) <= maxRecap) {
                        recapLines.push_back(remaining);
                        remaining.clear();
                    } else {
                        // Find last space within maxRecap chars for word break
                        int breakAt = maxRecap;
                        for (int j = maxRecap; j > 0; --j) {
                            if (remaining[j] == ' ') { breakAt = j; break; }
                        }
                        // If on the last allowed line and there's still more text, truncate
                        if (static_cast<int>(recapLines.size()) == maxLines - 1 &&
                            static_cast<int>(remaining.size()) > maxRecap) {
                            int trunc = breakAt > maxRecap - 3 ? maxRecap - 3 : breakAt;
                            recapLines.push_back(remaining.substr(0, trunc) + "...");
                            remaining.clear();
                        } else {
                            recapLines.push_back(remaining.substr(0, breakAt));
                            remaining = remaining.substr(breakAt);
                            // Trim leading space on next line
                            if (!remaining.empty() && remaining[0] == ' ')
                                remaining.erase(0, 1);
                        }
                    }
                }
                if (recapLines.empty()) recapLines.push_back("");
            }

            // First row: score + first recap line
            std::cout << color::c(color::dim) << "  \u2502" << color::c(color::reset)
                      << " " << awayStyle << g.away << " " << g.awayScore
                      << color::c(color::reset)
                      << color::c(color::dim) << "  @  " << color::c(color::reset)
                      << homeStyle << g.home << " " << g.homeScore
                      << color::c(color::reset)
                      << color::c(color::dim) << "  (" << g.status << ")"
                      << std::string(scorePad, ' ')
                      << color::c(color::reset);
            {
                int pad = recapCol - 1 - static_cast<int>(recapLines[0].size());
                if (pad < 0) pad = 0;
                std::cout << color::c(color::dim) << "\u2502"
                          << " " << recapLines[0] << std::string(pad, ' ')
                          << "\u2502" << color::c(color::reset) << "\n";
            }
            // Continuation rows: empty score column + wrapped recap
            for (size_t li = 1; li < recapLines.size(); ++li) {
                int pad = recapCol - 1 - static_cast<int>(recapLines[li].size());
                if (pad < 0) pad = 0;
                std::cout << color::c(color::dim) << "  \u2502"
                          << std::string(scoreCol, ' ') << "\u2502"
                          << " " << recapLines[li] << std::string(pad, ' ')
                          << "\u2502" << color::c(color::reset) << "\n";
            }
        }

        // Bottom border
        std::cout << color::c(color::dim) << "  \u2514"
                  << repeatStr(hl, scoreCol) << "\u2534"
                  << repeatStr(hl, recapCol) << "\u2518"
                  << color::c(color::reset) << "\n\n";
    }

    if (!anyGames) {
        std::cout << "  No East Coast games found today.\n";
    }

    std::cout << "\n" << color::c(color::dim) << "  More: https://www.espn.com" << color::c(color::reset) << "\n";
}

int main(int argc, char* argv[]) {
    // Disable color if stdout is not a terminal (e.g., piped to a file)
    if (!isatty(fileno(stdout))) {
        color::enabled = false;
    }

    // Parse command-line arguments
    std::string location;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--location" || arg == "-l") && i + 1 < argc) {
            location = argv[++i];
        } else if (arg == "--no-color") {
            color::enabled = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: dashboard [OPTIONS]\n\n"
                      << "Options:\n"
                      << "  -l, --location \"City, State\"   Set weather location (default: auto-detect)\n"
                      << "      --no-color                 Disable colored output\n"
                      << "  -h, --help                     Show this help message\n\n"
                      << "Examples:\n"
                      << "  ./dashboard\n"
                      << "  ./dashboard -l \"Denver, Colorado\"\n"
                      << "  ./dashboard --location \"Miami, FL\"\n";
            return 0;
        }
    }

    // Get current date
    std::time_t now = std::time(nullptr);
    char dateBuf[64];
    std::strftime(dateBuf, sizeof(dateBuf), "%A, %B %d, %Y", std::localtime(&now));

    std::cout << "\n";
    std::cout << color::c(color::bold) << color::c(color::cyan)
              << std::string(60, '=') << "\n";
    std::cout << "  TODAY'S DASHBOARD  --  " << dateBuf << "\n";
    std::cout << std::string(60, '=')
              << color::c(color::reset) << "\n\n";

    showWeather(location);
    printSeparator();

    showJoke();
    printSeparator();

    showNews();
    printSeparator();

    showSports();

    std::cout << color::c(color::bold) << color::c(color::cyan)
              << std::string(60, '=') << color::c(color::reset) << "\n\n";

    return 0;
}

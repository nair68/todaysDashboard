#include <iostream>
#include <string>
#include <array>
#include <cstdio>
#include <vector>
#include <ctime>
#include <algorithm>

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

void printSeparator() {
    std::cout << "\n" << std::string(60, '-') << "\n\n";
}

void showWeather(const std::string& location) {
    std::cout << "  TODAY'S WEATHER\n";
    std::cout << std::string(60, '-') << "\n";

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
        std::cout << "  Location:    " << loc << "\n";
        std::cout << "  Condition:   " << lines[1] << "\n";
        std::cout << "  Temperature: " << lines[2] << "\n";
        std::cout << "  Humidity:    " << lines[3] << "\n";
        std::cout << "  Wind:        " << lines[4] << "\n";
    } else {
        // Fallback: just print raw data
        std::cout << "  " << data << "\n";
    }
}

void showJoke() {
    std::cout << "  JOKE OF THE MOMENT\n";
    std::cout << std::string(60, '-') << "\n";

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
        std::cout << "  " << setup << "\n";
        std::cout << "  ... " << delivery << "\n";
    } else if (type == "single") {
        std::string joke = jsonValue(json, "joke");
        std::cout << "  " << joke << "\n";
    } else {
        std::cout << "  Could not parse joke.\n";
    }
}

void showNews() {
    std::cout << "  TODAY'S HEADLINES\n";
    std::cout << std::string(60, '-') << "\n";

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
        std::cout << "  " << (i + 1) << ". " << items[i] << "\n";
    }
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    std::string location;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--location" || arg == "-l") && i + 1 < argc) {
            location = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: dashboard [OPTIONS]\n\n"
                      << "Options:\n"
                      << "  -l, --location \"City, State\"   Set weather location (default: auto-detect)\n"
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
    std::cout << std::string(60, '=') << "\n";
    std::cout << "  TODAY'S DASHBOARD  --  " << dateBuf << "\n";
    std::cout << std::string(60, '=') << "\n\n";

    showWeather(location);
    printSeparator();

    showJoke();
    printSeparator();

    showNews();

    std::cout << "\n" << std::string(60, '=') << "\n\n";

    return 0;
}

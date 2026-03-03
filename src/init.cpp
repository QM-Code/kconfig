#include <kconfig/init.hpp>

#include <kconfig/store.hpp>
#include <ktrace/trace.hpp>
#include <spdlog/spdlog.h>

#include <cctype>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct ParsedAssignment {
    std::string name;
    std::string path;
    kconfig::json::Value value;
};

bool startsWith(const std::string_view value, const std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

std::string trimWhitespace(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return std::string(value);
}

bool hasUsableValueToken(const int index, const int argc, char** argv) {
    if (index + 1 >= argc || argv == nullptr) {
        return false;
    }
    const char* raw = argv[index + 1];
    if (raw == nullptr) {
        return false;
    }
    const std::string_view value(raw);
    if (value.empty()) {
        return false;
    }
    return value.front() != '-';
}

bool isQuoted(std::string_view value) {
    if (value.size() < 2) {
        return false;
    }
    const char quote = value.front();
    if ((quote != '"' && quote != '\'') || value.back() != quote) {
        return false;
    }
    return true;
}

std::string unquote(std::string_view value) {
    if (!isQuoted(value)) {
        return std::string(value);
    }

    std::string result;
    result.reserve(value.size() - 2);
    bool escaped = false;
    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        const char ch = value[i];
        if (escaped) {
            result.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        result.push_back(ch);
    }
    if (escaped) {
        result.push_back('\\');
    }
    return result;
}

std::size_t findUnquotedChar(std::string_view value, const char target) {
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool escaped = false;

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if ((inSingleQuote || inDoubleQuote) && ch == '\\') {
            escaped = true;
            continue;
        }
        if (!inDoubleQuote && ch == '\'') {
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (!inSingleQuote && ch == '"') {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        if (!inSingleQuote && !inDoubleQuote && ch == target) {
            return i;
        }
    }

    return std::string_view::npos;
}

std::string NormalizeConfigRoot(std::string_view config_root) {
    std::string root = trimWhitespace(config_root);
    if (root.empty()) {
        throw std::invalid_argument("config root must not be empty");
    }
    if (startsWith(root, "--")) {
        if (root.size() == 2) {
            throw std::invalid_argument("config root '--' is invalid");
        }
        return root;
    }
    if (!root.empty() && root.front() == '-') {
        throw std::invalid_argument("config root '" + root + "' must begin with '--' or no dashes");
    }
    return std::string("--") + root;
}

std::string normalizePath(std::string_view raw_path) {
    const std::string path = trimWhitespace(raw_path);
    if (path.empty()) {
        throw std::invalid_argument("config assignment path must not be empty");
    }

    std::string normalized;
    std::size_t offset = 0;
    bool first = true;

    while (offset < path.size()) {
        const std::size_t dot = path.find('.', offset);
        std::string segment = trimWhitespace(
            path.substr(offset, dot == std::string::npos ? std::string::npos : dot - offset));
        if (segment.empty()) {
            throw std::invalid_argument("config assignment path contains an empty segment");
        }

        if (isQuoted(segment)) {
            segment = unquote(segment);
        } else if (segment.front() == '"' || segment.front() == '\''
                   || segment.back() == '"' || segment.back() == '\'') {
            throw std::invalid_argument("config assignment path has invalid quoting");
        }

        if (segment.empty()) {
            throw std::invalid_argument("config assignment path segment must not be empty");
        }

        if (!first) {
            normalized.push_back('.');
        }
        normalized += segment;

        if (dot == std::string::npos) {
            break;
        }
        offset = dot + 1;
        first = false;
    }

    return normalized;
}

kconfig::json::Value parseValue(std::string_view raw_value) {
    const std::string text = trimWhitespace(raw_value);
    if (text.empty()) {
        throw std::invalid_argument("config assignment value must not be empty");
    }

    if (isQuoted(text) && text.front() == '\'') {
        return kconfig::json::Value(unquote(text));
    }

    try {
        return kconfig::json::Parse(text);
    } catch (...) {
        if (isQuoted(text)) {
            return kconfig::json::Value(unquote(text));
        }
        return kconfig::json::Value(text);
    }
}

ParsedAssignment ParseAssignment(std::string_view expression) {
    const std::string text = trimWhitespace(expression);
    if (text.empty()) {
        throw std::invalid_argument("config assignment must not be empty");
    }

    const std::size_t equal = findUnquotedChar(text, '=');
    if (equal == std::string::npos) {
        throw std::invalid_argument("config assignment must be '<namespace>.<path>=<value>'");
    }

    const std::string left = trimWhitespace(text.substr(0, equal));
    const std::string right = trimWhitespace(text.substr(equal + 1));
    if (left.empty() || right.empty()) {
        throw std::invalid_argument("config assignment must include both path and value");
    }

    const std::size_t dot = left.find('.');
    if (dot == std::string::npos) {
        throw std::invalid_argument("config assignment must include namespace and path");
    }

    ParsedAssignment assignment;
    assignment.name = trimWhitespace(left.substr(0, dot));
    if (assignment.name.empty()) {
        throw std::invalid_argument("config assignment namespace must not be empty");
    }
    for (const char ch : assignment.name) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            throw std::invalid_argument("config assignment namespace must not contain whitespace");
        }
    }

    assignment.path = normalizePath(left.substr(dot + 1));
    assignment.value = parseValue(right);
    return assignment;
}

void applyAssignmentOrThrow(const std::string_view option, std::string_view raw_assignment) {
    const auto assignment = ParseAssignment(raw_assignment);

    if (!kconfig::store::Get(assignment.name, ".")) {
        if (!kconfig::store::AddMutable(assignment.name, kconfig::json::Object())) {
            throw std::runtime_error(
                "failed to create mutable config namespace '" + assignment.name + "'");
        }
    }

    if (!kconfig::store::Set(assignment.name, assignment.path, assignment.value)) {
        throw std::runtime_error("failed to set '" + assignment.name + "." + assignment.path + "'");
    }

    KTRACE("cli",
           "option '{}' set '{}.{}'",
           option,
           assignment.name,
           assignment.path);
}

void printConfigHelp(const std::string& root) {
    std::cout
        << "\nKConfig options:\n"
        << "  " << root << " <assignment>      Set one value (for example app.\"theme\"=\"dark\")\n"
        << "  " << root << "-help              Show this help\n"
        << "  " << root << "-examples          Show assignment examples\n\n";
}

void printConfigExamples(const std::string& root) {
    std::cout
        << "\nKConfig examples:\n"
        << "  " << root << " 'client.\"Language\"=\"en\"'\n"
        << "  " << root << " 'graphics.width=1920'\n"
        << "  " << root << " 'graphics.fullscreen=true'\n"
        << "  " << root << " 'audio.devices[0]=\"default\"'\n\n";
}

void processCliArgs(int& argc, char** argv, std::string_view config_root) {
    if (argc <= 0 || argv == nullptr) {
        return;
    }

    const std::string root = NormalizeConfigRoot(config_root);
    const std::string root_help = root + "-help";
    const std::string root_examples = root + "-examples";
    std::vector<bool> consumed(static_cast<std::size_t>(argc), false);

    KTRACE("cli",
           "processing CLI options (enable kconfig.cli for details): {} arg(s), root '{}'",
           argc,
           root);

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }

        const std::string arg = trimWhitespace(std::string(argv[i]));
        if (arg.empty() || !startsWith(arg, root)) {
            continue;
        }

        if (arg == root_help) {
            consumed[static_cast<std::size_t>(i)] = true;
            printConfigHelp(root);
            KTRACE("cli", "handled '{}'", root_help);
            continue;
        }

        if (arg == root_examples) {
            consumed[static_cast<std::size_t>(i)] = true;
            printConfigExamples(root);
            KTRACE("cli", "handled '{}'", root_examples);
            continue;
        }

        if (arg == root) {
            consumed[static_cast<std::size_t>(i)] = true;
            if (!hasUsableValueToken(i, argc, argv)) {
                printConfigHelp(root);
                KTRACE("cli", "'{}' had no value, showed help", root);
                continue;
            }

            consumed[static_cast<std::size_t>(i + 1)] = true;
            try {
                applyAssignmentOrThrow(root, argv[++i]);
            } catch (const std::exception& ex) {
                spdlog::error("\nConfig option error: {}", ex.what());
                printConfigExamples(root);
            }
            continue;
        }

        consumed[static_cast<std::size_t>(i)] = true;
        KTRACE("cli", "consumed unknown config option '{}'", arg);
    }

    int write_index = 1;
    for (int read_index = 1; read_index < argc; ++read_index) {
        if (!consumed[static_cast<std::size_t>(read_index)]) {
            argv[write_index++] = argv[read_index];
        }
    }
    if (write_index < argc) {
        argv[write_index] = nullptr;
    }
    argc = write_index;
}

void ParseConfigCLI(int& argc, char** argv, std::string_view config_root) {
    try {
        processCliArgs(argc, argv, config_root);
    } catch (const std::exception& ex) {
        std::string root = "--config";
        try {
            root = NormalizeConfigRoot(config_root);
        } catch (...) {
        }
        spdlog::error("\nConfig option error: {}", ex.what());
        printConfigExamples(root);
    }
}

void RegisterKConfigChannels() {
    ktrace::RegisterChannel("config", ktrace::Color("DeepSkyBlue1"));
    ktrace::RegisterChannel("config.requests", ktrace::Color("LightGoldenrod2"));
    ktrace::RegisterChannel("data", ktrace::Color("MediumSpringGreen"));
    ktrace::RegisterChannel("cli", ktrace::Color("Orange3"));
    ktrace::RegisterChannel("content", ktrace::Color("MediumOrchid1"));
}

std::once_flag g_trace_init_once;

} // namespace

namespace kconfig::init {

void Initialize() {
    std::call_once(g_trace_init_once, RegisterKConfigChannels);
}

void ParseCLI(int& argc, char** argv, std::string_view config_root) {
    ParseConfigCLI(argc, argv, config_root);
}

} // namespace kconfig::init

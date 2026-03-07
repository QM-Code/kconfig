#include "../store.hpp"

#include "api_impl.hpp"
#include <kconfig/store.hpp>

#include <ktrace.hpp>

#include <cctype>
#include <string>
#include <string_view>

namespace {

std::string trimWhitespace(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return std::string(value);
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

bool parseCliConfigValue(std::string_view rawValue, kconfig::json::Value& outValue, std::string* error) {
    const std::string text = trimWhitespace(rawValue);
    if (text.empty()) {
        if (error) {
            *error = "config assignment value must not be empty";
        }
        return false;
    }

    if (isQuoted(text) && text.front() == '\'') {
        outValue = kconfig::json::Value(unquote(text));
        return true;
    }

    try {
        outValue = kconfig::json::Parse(text);
        return true;
    } catch (...) {
        if (isQuoted(text)) {
            outValue = kconfig::json::Value(unquote(text));
        } else {
            outValue = kconfig::json::Value(text);
        }
        return true;
    }
}

bool parseCliConfigAssignment(std::string_view text,
                              std::string& outPath,
                              kconfig::json::Value& outValue,
                              std::string* error) {
    const std::string expression = trimWhitespace(text);
    if (expression.empty()) {
        if (error) {
            *error = "config assignment must not be empty";
        }
        return false;
    }

    const std::size_t equal = findUnquotedChar(expression, '=');
    if (equal == std::string::npos) {
        if (error) {
            *error = "config assignment must be '\"<key>\"=<value>'";
        }
        return false;
    }

    const std::string left = trimWhitespace(expression.substr(0, equal));
    const std::string right = trimWhitespace(expression.substr(equal + 1));
    if (left.empty() || right.empty()) {
        if (error) {
            *error = "config assignment must include both key and value";
        }
        return false;
    }
    if (!isQuoted(left)) {
        if (error) {
            *error = "config assignment key must be quoted: '\"<key>\"=<value>'";
        }
        return false;
    }

    outPath = trimWhitespace(unquote(left));
    if (outPath.empty()) {
        if (error) {
            *error = "config assignment key must not be empty";
        }
        return false;
    }

    if (!parseCliConfigValue(right, outValue, error)) {
        return false;
    }

    // Validate key format and path semantics against the store path setter.
    kconfig::json::Value probe = kconfig::json::Object();
    if (!kconfig::store::setValueAtPath(probe, outPath, outValue)) {
        if (error) {
            *error = "config assignment key path is invalid";
        }
        return false;
    }

    return true;
}

} // namespace

namespace kconfig::store::api {

bool StoreCliConfig(std::string_view name, std::string_view text, std::string* error) {
    KTRACE("store",
           "StoreCliConfig requested: namespace='{}' text='{}' (enable store.requests for details)",
           std::string(name),
           std::string(text));
    KTRACE("store.requests",
           "StoreCliConfig namespace='{}' text='{}'",
           std::string(name),
           std::string(text));

    if (name.empty()) {
        if (error) {
            *error = "namespace must not be empty";
        }
        return false;
    }
    for (const char ch : name) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            if (error) {
                *error = "namespace must not contain whitespace";
            }
            return false;
        }
    }

    std::string path;
    kconfig::json::Value value;
    if (!parseCliConfigAssignment(text, path, value, error)) {
        return false;
    }

    if (!Get(name, ".")) {
        if (!AddMutable(name, kconfig::json::Object())) {
            if (error) {
                *error = "failed to create mutable config namespace '" + std::string(name) + "'";
            }
            return false;
        }
    }

    if (!Set(name, path, std::move(value))) {
        if (error) {
            *error = "failed to set '" + std::string(name) + "." + path + "'";
        }
        return false;
    }

    KTRACE("store.requests",
           "StoreCliConfig namespace='{}' key='{}' -> stored",
           std::string(name),
           path);
    return true;
}

} // namespace kconfig::store::api

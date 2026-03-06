#include <kconfig.hpp>

#include <kcli.hpp>
#include <ktrace.hpp>

#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
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

void applyAssignmentOrThrow(const std::string_view option,
                            std::string_view configNamespace,
                            std::string_view rawAssignment) {
    std::string error;
    if (!kconfig::store::StoreCliConfig(configNamespace, rawAssignment, &error)) {
        throw std::runtime_error("failed to store CLI config from option '"
                                 + std::string(option)
                                 + "': "
                                 + error);
    }

    KTRACE("cli",
           "option '{}' stored assignment in namespace '{}': {}",
           option,
           std::string(configNamespace),
           std::string(rawAssignment));
}

void applyUserConfigPathOrThrow(const std::string_view option, std::string_view raw_path) {
    const std::string trimmedPath = trimWhitespace(raw_path);
    if (trimmedPath.empty()) {
        throw std::invalid_argument("user config path must not be empty");
    }

    if (!kconfig::store::SetUserConfigFilePath(std::filesystem::path(trimmedPath))) {
        throw std::runtime_error("failed to set user config path from option '" + std::string(option) + "'");
    }

    KTRACE("cli",
           "option '{}' set user config path '{}'",
           option,
           std::filesystem::path(trimmedPath).string());
}

void printConfigExamples(const std::string& root) {
    std::cout
        << "\nKConfig examples:\n"
        << "  " << root << " '\"client.Language\"=\"en\"'\n"
        << "  " << root << " '\"graphics.width\"=1920'\n"
        << "  " << root << " '\"graphics.fullscreen\"=true'\n"
        << "  " << root << " '\"audio.devices[0]\"=\"default\"'\n\n";
}

void processCliArgs(int& argc, char** argv, std::string_view config_root) {
    if (argc <= 0 || argv == nullptr) {
        return;
    }

    kcli::Parser cli;
    cli.Initialize(argc, argv, config_root);
    const std::string root = std::string("--") + cli.GetRoot();
    const std::string rootNamespace = cli.GetRoot();
    const std::string root_examples = root + "-examples";
    const std::string root_user = root + "-user";

    KTRACE("cli",
           "processing CLI options (enable kconfig.cli for details): {} arg(s), root '{}'",
           argc,
           root);

    cli.SetRootValueHandler(
        [&](const kcli::HandlerContext&, std::string_view value) {
            applyAssignmentOrThrow(root, rootNamespace, value);
        },
        "<\"key\"=<value>>",
        "Set one value in the root namespace.");

    cli.Implement("examples",
                  [&](const kcli::HandlerContext&) {
                      printConfigExamples(root);
                      KTRACE("cli", "handled '{}'", root_examples);
                  },
                  "Show assignment examples.");

    cli.Implement("user",
                  [&](const kcli::HandlerContext&, std::string_view value) {
                      applyUserConfigPathOrThrow(root_user, value);
                  },
                  "Override user config file path for LoadUserConfigFile.");

    (void)cli.Process();
}

} // namespace

namespace kconfig {

void ParseCLI(int& argc, char** argv, std::string_view config_root) {
    processCliArgs(argc, argv, config_root);
}

} // namespace kconfig

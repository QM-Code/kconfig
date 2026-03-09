#include <kconfig.hpp>

#include <ktrace.hpp>

namespace kconfig {

ktrace::TraceLogger GetTraceLogger() {
    ktrace::TraceLogger logger;
    logger.addChannel("config", ktrace::Color("DeepSkyBlue1"));
    logger.addChannel("store", ktrace::Color("LightGoldenrod2"));
    logger.addChannel("store.requests");
    logger.addChannel("asset", ktrace::Color("SteelBlue1"));
    logger.addChannel("asset.requests");
    logger.addChannel("io", ktrace::Color("MediumSpringGreen"));
    logger.addChannel("cli", ktrace::Color("Orange3"));
    logger.addChannel("content", ktrace::Color("MediumOrchid1"));
    return logger;
}

} // namespace kconfig

#include "server/runtime.hpp"

#include "karma/app/server/runner.hpp"

int main(int argc, char** argv) {
    karma::app::server::RunSpec run_spec{};
    run_spec.parse_app_name = "bz3-server";
    run_spec.bootstrap_app_name = "bz3";
    return karma::app::server::Run(argc, argv, run_spec, bz3::server::RunRuntime);
}

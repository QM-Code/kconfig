#include "client/runtime.hpp"

#include "karma/app/client/runner.hpp"

int main(int argc, char** argv) {
    karma::app::client::RunSpec run_spec{};
    run_spec.parse_app_name = "bz3";
    run_spec.bootstrap_app_name = "bz3";
    return karma::app::client::Run(argc, argv, run_spec, bz3::client::RunRuntime);
}

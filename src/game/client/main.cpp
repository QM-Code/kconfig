#include "client/runtime.hpp"

#include "karma/app/client_runner.hpp"

int main(int argc, char** argv) {
    karma::app::ClientRunSpec run_spec{};
    run_spec.parse_app_name = "bz3";
    run_spec.bootstrap_app_name = "bz3";
    return karma::app::RunClient(argc, argv, run_spec, bz3::client::RunRuntime);
}

#include <kconfig.hpp>

#include <iostream>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    kconfig::Initialize();
    std::cout << "Bootstrap succeeded.\n";
    return 0;
}

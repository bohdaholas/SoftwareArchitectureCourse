// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <csignal>
#include "facade_controller.h"
#include "options_parser.h"

using std::cout, std::cerr, std::endl;

void handle_sigterm() {
    cout << "--- FacadeService is off!" << endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    struct sigaction sa{};
    std::string filename;
    if (argc == 1) {
        filename = "../config.cfg";
    } else if (argc == 2) {
        filename = argv[1];
    } else {
        cerr << "Wrong number of arguments" << endl;
        exit(EXIT_FAILURE);
    }

    config_options_t opt{filename};

    sa.sa_handler = reinterpret_cast<__sighandler_t>(&handle_sigterm);
    sigaction(SIGTERM, &sa, nullptr);

    FacadeController facade_controller(opt);
    facade_controller.run();
}

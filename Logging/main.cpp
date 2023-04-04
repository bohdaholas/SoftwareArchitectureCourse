// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include "logging_controller.h"

using std::cout, std::cerr, std::endl;

int main(int argc, char *argv[]) {
    Server app;
    return app.run(argc, argv);
}

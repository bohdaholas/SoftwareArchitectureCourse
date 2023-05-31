// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <filesystem>
#include <iostream>
#include <fstream>
#include "options_parser.h"

using std::cout;
using std::cerr;
using std::endl;

namespace po = boost::program_options;

config_options_t::config_options_t() {
    opt_conf.add_options()
            ("DEFAULT.start_port", po::value<int>(&start_port)->required())
            ("MQ_NAME.mq_name", po::value<std::string>(&mq_name)->required())
            ;
}

config_options_t::config_options_t(const std::string &filename):
        config_options_t() // Delegate constructor
{
    parse(filename);
}

void config_options_t::parse(const std::string &filename) {
    std::ifstream cfgFile(filename);
    if (!cfgFile.is_open()) {
        cerr << "Failed to open config file" << endl;
        exit(EXIT_FAILURE);
    }
    try {
        po::parsed_options parsed = po::parse_config_file(cfgFile, opt_conf);
        po::store(parsed, var_map);
        po::notify(var_map);
    } catch (std::exception &ex) {
        cerr << "Failed configuration: " << ex.what() << endl;
        exit(EXIT_FAILURE);
    }
}

#ifndef MYCAT_CONFIG_FILE_H
#define MYCAT_CONFIG_FILE_H

#include <boost/program_options.hpp>
#include <string>
#include <exception>
#include <stdexcept>

class config_options_t {
public:
    config_options_t();
    explicit config_options_t(const std::string &filename);

    //! Explicit is better than implicit:
    config_options_t(const config_options_t&) = default;
    config_options_t& operator=(const config_options_t&) = default;
    config_options_t(config_options_t&&) = default;
    config_options_t& operator=(config_options_t&&) = default;
    ~config_options_t() = default;

    void parse(const std::string &filename);

    int start_port;
    std::string mq_name;

private:
    boost::program_options::variables_map var_map{};
    boost::program_options::options_description opt_conf{};
};

#endif //MYCAT_CONFIG_FILE_H


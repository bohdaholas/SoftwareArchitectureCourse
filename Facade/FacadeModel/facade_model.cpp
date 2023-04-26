// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <sstream>
#include <iostream>
#include <random>
#include "facade_model.h"

using std::cout, std::cerr, std::endl;

ServiceManager &ServiceManager::get_instance() {
    static ServiceManager instance;
    return instance;
}

std::string ServiceManager::get_name(int fd) const {
    if (socket_name_map.contains(fd)) {
        return socket_name_map.at(fd);
    }
}

void ServiceManager::set_name(int fd, const std::string& name) {
    socket_name_map[fd] = name;
}

Counter &Counter::get_instance() {
    static Counter instance;
    return instance;
}

void Counter::increment() {
    ++counter;
}

int Counter::get_count() const {
    return counter;
}

Message::Message(const std::string &msg) : message{msg}, uuid(this->generate_uuid(msg)) {}

Message::Message(const std::string &uuid, const std::string &msg) : message{msg}, uuid(uuid) {}

std::string Message::generate_uuid(const std::string &string) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10e3, 10e8);
    size_t request_hash{std::hash<uint64_t>{}(dis(gen))};

    std::ostringstream ss;
    ss << std::hex << request_hash;

    const size_t hash_len = 5;
    return ss.str().substr(0, hash_len);
}

std::string Message::str() const {
    std::ostringstream oss;
    oss << uuid << "=" << message;
    return oss.str();
}

const std::string &Message::get_message() const {
    return message;
}

const std::string &Message::get_uuid() const {
    return uuid;
}

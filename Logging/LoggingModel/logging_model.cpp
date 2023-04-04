#include <sstream>
#include <random>
#include "logging_model.h"

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

#ifndef FACADE_SERVICE_MESSAGE_H
#define FACADE_SERVICE_MESSAGE_H

#include <iostream>
#include <string>
#include <random>

class Message {
public:
    Message(const std::string& msg) : message{msg}, uuid(this->generate_uuid(msg)) {}

    void generate_uuid() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(10e3, 10e8);
        size_t request_hash{std::hash<uint64_t>{}(dis(gen))};

        std::ostringstream ss;
        ss << std::hex << request_hash;

        const size_t hash_len = 5;
        return ss.str().substr(0, hash_len);
    }

    std::string str() {
        std::ostringstream oss;
        oss << uuid << "=" << message;
        return oss.str();
    }

private:
    std::string message;
    std::string uuid;
};

#endif //FACADE_SERVICE_MESSAGE_H

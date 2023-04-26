// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef _FACADE_MODEL_H
#define _FACADE_MODEL_H

#include <string>
#include <future>
#include <unordered_map>

class ServiceManager {
public:
    static ServiceManager& get_instance();
    std::string get_name(int fd) const;
    void set_name(int fd, const std::string& name);
private:
    ServiceManager() = default;
    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;

    std::unordered_map<int, std::string> socket_name_map;
};

class Counter {
public:
    static Counter& get_instance();
    void increment();
    [[nodiscard]] int get_count() const;
private:
    Counter() = default;
    Counter(const Counter&) = delete;
    Counter& operator=(const Counter&) = delete;
    int counter = 0;
};

class Message {
public:
    explicit Message(const std::string& msg);
    Message(const std::string& uuid, const std::string& msg);

    std::string generate_uuid(const std::string& string);

    [[nodiscard]] std::string str() const;
    [[nodiscard]] const std::string &get_message() const;
    [[nodiscard]] const std::string &get_uuid() const;
private:
    std::string message;
    std::string uuid;
};

#endif //_FACADE_MODEL_H

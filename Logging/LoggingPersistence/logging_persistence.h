#ifndef _LOGGING_PERSISTENCE_H_
#define _LOGGING_PERSISTENCE_H_

#include <vector>
#include <unordered_map>
#include <hazelcast/client/hazelcast_client.h>
#include <hazelcast/client/imap.h>
#include "logging_model.h"

class LoggingPersistence {
public:
    virtual void save_message(const Message& msg) = 0;
    virtual std::vector<Message> get_messages() = 0;
};

class InMemoryPersistence : public LoggingPersistence {
public:
    void save_message(const Message &msg) override;
    std::vector<Message> get_messages() override;

private:
    std::unordered_map<std::string, std::string> messages_map;
};

class HazelcastPersistence : public LoggingPersistence {
public:
    explicit HazelcastPersistence(std::string map_name);

    void save_message(const Message &msg) override;
    std::vector<Message> get_messages() override;

private:
    std::string map_name;
};

#endif
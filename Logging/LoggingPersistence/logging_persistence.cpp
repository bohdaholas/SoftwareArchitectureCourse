#include <iostream>
#include <utility>
#include "logging_persistence.h"

using std::cout, std::cerr, std::endl;

void InMemoryPersistence::save_message(const Message &msg) {
    messages_map[msg.get_uuid()] = msg.get_message();
}

std::vector<Message> InMemoryPersistence::get_messages() {
    std::vector<Message> messages_list;
    messages_list.reserve(messages_map.size());
    for (const auto &[uuid, message_str]: messages_map) {
        Message message(uuid, message_str);
        messages_list.push_back(message);
    }
    return messages_list;
}

HazelcastPersistence::HazelcastPersistence(const std::string& map_name) : map_name{map_name} {
    clint_config.get_logger_config().level(hazelcast::logger::level::off);
    hz_client = std::make_shared<hazelcast::client::hazelcast_client>(hazelcast::new_client(std::move(clint_config)).get());
    map = hz_client->get_map(map_name).get();
}

void HazelcastPersistence::save_message(const Message &msg) {
    cout << msg.str() << endl;
    map->put<std::string, std::string>(msg.get_uuid(), msg.get_message()).get();
}

std::vector<Message> HazelcastPersistence::get_messages() {
    auto entries = map->entry_set<std::string, std::string>().get();
    std::vector<Message> res;
    res.reserve(entries.size());
    for (auto const& [uuid, msg] : entries) {
        res.emplace_back(uuid, msg);
    }
    return res;
}

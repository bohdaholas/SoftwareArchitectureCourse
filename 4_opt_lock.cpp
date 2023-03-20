#include <hazelcast/client/hazelcast_client.h>
#include <iostream>

int main() {
    auto hz = hazelcast::new_client().get();
    std::cout << "Created HZ instance" << std::endl;

    auto map = hz.get_map("4_dis_map").get();
    std::cout << "Getting map from HZ instance" <<  std::endl
              << "Setting key to 0 & start incrementing it..." << std::endl;

    const std::string key = "value";
    map->put<std::string, int>(key, 0);
    constexpr int IT_NUM = 1000;

    for (int i = 0; i < IT_NUM; ++i) {
        int old_value = map->get<std::string, int>(key).get().get();
        int new_value = old_value + 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        map->replace<std::string, int>(key, old_value, new_value).get();
    }
    int final_key_value = map->get<std::string, int>(key).get().get();
    std::cout << "After " << IT_NUM << " iterations the key value is " << final_key_value << std::endl;

    return 0;
}

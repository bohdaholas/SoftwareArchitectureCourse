#include <hazelcast/client/hazelcast_client.h>
#include <iostream>

int main() {
    auto hz = hazelcast::new_client().get();
    std::cout << "Created HZ instance" << std::endl;

    auto map = hz.get_map("3_dis_map").get();
    std::cout << "Getting map from HZ instance" <<  std::endl
              << "Start filling the map..." << std::endl;

    for (int i = 0; i < 1000; ++i) {
        map->put<int, int>(i, i).get();
    }
    std::cout << "Finished filling the map!" << std::endl;

    return 0;
}
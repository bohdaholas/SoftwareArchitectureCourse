#include <hazelcast/client/hazelcast_client.h>
#include <iostream>

int main() {
    auto hz = hazelcast::new_client().get();

    auto queue = hz.get_queue("queue").get();

    std::cout << "Start consuming..." << std::endl;
    for (int it = 1; it < 20; it++) {
        auto k = queue->take<int>().get();
        std::cout << "consumed k=" << k.get() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    std::cout << "Stopped consuming" << std::endl;

    return 0;
}

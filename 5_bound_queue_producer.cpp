#include <hazelcast/client/hazelcast_client.h>
#include <iostream>

int main() {
    auto hz = hazelcast::new_client().get();

    auto queue = hz.get_queue("queue").get();

    std::cout << "Start producing..." << std::endl;
    for (int k = 1; k < 20; k++) {
        queue->put(k).get();
        std::cout << "produced k=" << k << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds (1));
    }
    queue->put(-1).get();
    std::cout << "Stopped producing" << std::endl;

    return 0;
}

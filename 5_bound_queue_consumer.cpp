#include <hazelcast/client/hazelcast_client.h>
#include <iostream>

int main() {
    auto hz = hazelcast::new_client().get();

    auto queue = hz.get_queue("queue").get();

    std::cout << "Start consuming..." << std::endl;
    for (int k = 1; k < 100; k++) {
        queue->put(k).get();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    queue->put(-1).get();
    std::cout << "Stopped consuming" << std::endl;

    return 0;
}

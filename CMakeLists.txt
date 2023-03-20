cmake_minimum_required(VERSION 3.20)
project(task2)

set(CMAKE_CXX_STANDARD 23)

set(hazelcast-cpp-client_DIR /usr/lib/lib/cmake/hazelcast-cpp-client)
find_package(hazelcast-cpp-client CONFIG REQUIRED)

add_executable(3_dis_map 3_dis_map.cpp)
add_executable(4_no_lock 4_no_lock.cpp)
add_executable(4_opt_lock 4_opt_lock.cpp)
add_executable(4_pes_lock 4_pes_lock.cpp)
add_executable(5_bound_queue_producer 5_bound_queue_producer.cpp)
add_executable(5_bound_queue_consumer 5_bound_queue_consumer.cpp)

target_link_libraries(3_dis_map PRIVATE hazelcast-cpp-client::hazelcast-cpp-client)
target_link_libraries(4_no_lock PRIVATE hazelcast-cpp-client::hazelcast-cpp-client)
target_link_libraries(4_pes_lock PRIVATE hazelcast-cpp-client::hazelcast-cpp-client)
target_link_libraries(4_opt_lock PRIVATE hazelcast-cpp-client::hazelcast-cpp-client)
target_link_libraries(5_bound_queue_producer PRIVATE hazelcast-cpp-client::hazelcast-cpp-client)
target_link_libraries(5_bound_queue_consumer PRIVATE hazelcast-cpp-client::hazelcast-cpp-client)

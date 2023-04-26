cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME facade_service)
project(${PROJECT_NAME} C CXX)

set(CMAKE_CXX_STANDARD 23)


file(GLOB_RECURSE SOURCES "FacadeModel/*.cpp" "FacadeService/*.cpp"
        "FacadeController/*.cpp"  "options_parser/*.cpp" "main.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE FacadeModel FacadeController
                                                   FacadeService options_parser)

set(hazelcast-cpp-client_DIR /usr/lib/lib/cmake/hazelcast-cpp-client)
find_package(hazelcast-cpp-client CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE hazelcast-cpp-client::hazelcast-cpp-client)

find_package(Boost 1.71.0 COMPONENTS program_options system thread REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::program_options Boost::system Boost::thread)


cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME logging_microservice)
project(${PROJECT_NAME} C CXX)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "LoggingModel/*.cpp" "LoggingService/*.cpp"
                          "LoggingController/*.cpp" "LoggingPersistence/*.cpp" "options_parser/*.cpp" "main.cpp")
include_directories(LoggingController LoggingModel LoggingService LoggingPersistence options_parser)

set(hazelcast-cpp-client_DIR /usr/lib/lib/cmake/hazelcast-cpp-client)

add_executable(logging_microservice ${SOURCES})

find_package(hazelcast-cpp-client CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} hazelcast-cpp-client::hazelcast-cpp-client)

find_package(Boost 1.71.0 COMPONENTS program_options REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} Boost::program_options)

find_package(Poco 1.12.4 COMPONENTS Net Util REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Poco_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} Poco::Net Poco::Util)

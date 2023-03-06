cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME facade_service)
project(${PROJECT_NAME} C CXX)

set(CMAKE_CXX_STANDARD 23)

add_executable(${PROJECT_NAME} src/main.cpp src/options_parser.cpp src/server.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE include)

find_package(Boost 1.71.0 COMPONENTS program_options system REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} Boost::program_options Boost::system)

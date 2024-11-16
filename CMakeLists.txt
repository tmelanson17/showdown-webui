project(websocket_client)

add_subdirectory(json)
add_subdirectory(shared_queue)
add_subdirectory(state_machine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_minimum_required(VERSION 3.17)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS system thread)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Add main.cpp and user_login.cpp to the project
add_executable(user_login main.cpp user_login.cpp)

# Include nlohmann_json and boost::beast.
# TODO : Split libraries and main function.
target_include_directories(user_login PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})
target_link_libraries(user_login PUBLIC 
    nlohmann_json::nlohmann_json 
    OpenSSL::SSL
    OpenSSL::Crypto
    shared_queue 
    state_machine 
    ${Boost_LIBRARIES})

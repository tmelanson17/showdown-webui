# Define the interface library
add_library(shared_queue INTERFACE)

# Specify the include directories for the interface library
target_include_directories(shared_queue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
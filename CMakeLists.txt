cmake_minimum_required(VERSION 3.0)
project(MindfulnessDaemon)

# Find required libraries
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNOTIFY REQUIRED IMPORTED_TARGET libnotify)

# Set sources
set(SOURCES main.cpp)

# Add executable
add_executable(mindfulness_daemon ${SOURCES})

# Link libraries
target_link_libraries(mindfulness_daemon PRIVATE CURL::libcurl PkgConfig::LIBNOTIFY)

# Install
install(TARGETS mindfulness_daemon DESTINATION bin)
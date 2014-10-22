# Additional libraries
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libraries/RadioHead)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Path to your arduino-cmake directory
set(CMAKE_ARDUINO_PATH ~/dev/Arduino/arduino-cmake)
set(CMAKE_MODULE_PATH ${CMAKE_ARDUINO_PATH}/cmake/modules) # CMake module search path
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_ARDUINO_PATH}/cmake/ArduinoToolchain.cmake) # Arduino Toolchain

# Set the minimum required version of cmake for your project
cmake_minimum_required(VERSION 2.8)

# Set a variable for commands below
set(PROJECT_NAME server)

# Define your project and language
project(${PROJECT_NAME} C CXX)

# Load settings for external arduino-cmake project
# find_package(Arduino)
# set(ARDUINO_SDK_PATH )

# Define your Arduino board (below is for Arduino Duemilanova)
set(${PROJECT_NAME}_BOARD pro328)

# Define the source code
set(${PROJECT_NAME}_SRCS server.cpp)

# Define the port for uploading code to the Arduino
set(${PROJECT_NAME}_PORT /dev/cu.usbserial-A703KYPS)

# Command to generate code arduino firmware (.hex file)
generate_arduino_firmware(${PROJECT_NAME})

# link_directories(/Users/martinb/dev/Arduino/libraries)
cmake_minimum_required(VERSION 3.23)

project(ImageProcessing)

set(CMAKE_CXX_STANDARD 20)
find_package(OpenCV REQUIRED)

add_executable(Server Server.cpp)
add_executable(Client Client.cpp)

include_directories( ${OpenCV_INCLUDE_DIRS})

target_link_libraries( Client ${OpenCV_LIBS} )

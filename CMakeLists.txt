# CMakeList.txt
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

project ("Mail")

find_package(Boost REQUIRED)

add_subdirectory(POP3ServiceCore)
add_subdirectory(POP3Server)



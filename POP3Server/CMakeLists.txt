
set(PROJECT_NAME POP3Server)
set(EXECUTABLE_NAME POP3Server)

### Paths to directories w headers & sources
set(${PROJECT_NAME}_HEADERS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(${PROJECT_NAME}_SOURCES_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/source")

### List headers & sources files
file(GLOB ${PROJECT_NAME}_HEADERS "${${PROJECT_NAME}_HEADERS_DIRECTORY}/*.h")
file(GLOB ${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES_DIRECTORY}/*.cpp")

### For VS
source_group("include" FILES ${${PROJECT_NAME}_HEADERS})
source_group("source" FILES ${${PROJECT_NAME}_SOURCES})

add_executable(${EXECUTABLE_NAME} ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})

### Include directories w headers
target_include_directories(POP3Server PRIVATE "include")
target_include_directories(POP3Server PRIVATE "${CMAKE_SOURCE_DIR}/POP3Common")
target_include_directories(POP3Server PRIVATE "${CMAKE_SOURCE_DIR}/POP3ServiceCore/include")
target_include_directories(POP3Server PRIVATE "${Boost_INCLUDE_DIRS}")

### Link addiitonal libs
target_link_libraries(POP3Server PRIVATE POP3ServiceCore)
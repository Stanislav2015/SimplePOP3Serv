
set(PROJECT_NAME POP3ServiceCore)
set(LIBRARY_NAME ${PROJECT_NAME})
project(${PROJECT_NAME})

### Paths to directories w headers & sources
set(${PROJECT_NAME}_HEADERS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(${PROJECT_NAME}_SOURCES_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/source")

### List headers & sources files
file(GLOB ${PROJECT_NAME}_HEADERS "${${PROJECT_NAME}_HEADERS_DIRECTORY}/*.h")
file(GLOB ${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES_DIRECTORY}/*.cpp")

### For VS
source_group("include" FILES ${${PROJECT_NAME}_HEADERS})
source_group("source" FILES ${${PROJECT_NAME}_SOURCES})

### Add library
add_library(${LIBRARY_NAME} STATIC ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
target_include_directories(${LIBRARY_NAME} PUBLIC "include")

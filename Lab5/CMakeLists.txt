cmake_minimum_required(VERSION 3.10)
project(WebServerApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME} 
    Source.cpp
    Server/Server.cpp
    Server/ClientSession.cpp
    ThreadPool/ThreadPool.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE Server ThreadPool)

target_link_libraries(${PROJECT_NAME} ws2_32)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pages DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

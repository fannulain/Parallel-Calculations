cmake_minimum_required(VERSION 3.10)
project(ParallelCalculations_Lab4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(ServerApp 
    Server/Source.cpp
    Server/Server.cpp
    Server/ClientHandler.cpp
    Server/TaskHandler.cpp
)

target_include_directories(ServerApp PRIVATE Server)
target_link_libraries(ServerApp ws2_32)


add_executable(ClientApp 
    Client_CPP/Source.cpp
    Client_CPP/Client.cpp
    Client_CPP/EventHandler.cpp
)

target_include_directories(ClientApp PRIVATE Client_CPP)
target_link_libraries(ClientApp ws2_32)
#include "Server.h"
#include "ClientSession.h"
#include <iostream>

Server::Server(int port) : port(port), listenSocket(INVALID_SOCKET), isRunning(false) 
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) 
    {
        throw std::runtime_error("WSAStartup failed with error: " + std::to_string(result));
    }
}

Server::~Server() 
{
    stop();
    WSACleanup();
}

void Server::stop() 
{
    isRunning = false;
    if (listenSocket != INVALID_SOCKET) 
    {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }
}

void Server::start() 
{
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) 
    {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        std::cerr << "bind() failed with error: " << WSAGetLastError() << std::endl;
        stop();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) 
    {
        std::cerr << "listen() failed with error: " << WSAGetLastError() << std::endl;
        stop();
        return;
    }

    std::cout << "Server listening on port " << port << "..." << std::endl;
    isRunning = true;

    acceptConnections();
}

void Server::acceptConnections()
{
    while (isRunning) 
    {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) 
        {
            if (isRunning) 
            {
                std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
            }
            break;
        }

        std::cout << "Client connected." << std::endl;

        std::thread clientThread([](SOCKET socket) {
            ClientSession session(socket);
            session.process();
        }, clientSocket);

        clientThread.detach();
    }
}

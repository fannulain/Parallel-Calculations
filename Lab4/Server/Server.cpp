#include "Server.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

Server::Server(uint16_t port) : port(port), serverSocket(INVALID_SOCKET), isRunning(false)
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) 
    {
        throw std::runtime_error("[Server] WSAStartup failed with error: " + std::to_string(result));
    }
}

Server::~Server()
{
    stop();
    WSACleanup();
}

void Server::start()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) 
    {
        throw std::runtime_error("[Server] Socket creation failed with error: " + std::to_string(WSAGetLastError()));
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;

    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
    {
        int err = WSAGetLastError();
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        throw std::runtime_error("[Server] Bind failed with error: " + std::to_string(err));
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) 
    {
        int err = WSAGetLastError();
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        throw std::runtime_error("[Server] Listen failed with error: " + std::to_string(err));
    }

    isRunning = true;
    std::cout << "[Server] Server started. Listening on port " << port << "...\n";

    acceptLoop();
}

void Server::acceptLoop()
{
    while (isRunning) 
    {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) 
        {
            if (isRunning) 
            {
                std::cerr << "[Server] Accept failed with error: " << WSAGetLastError() << "\n";
            }
            continue;
        }

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
        std::cout << "[Server] New client connected: " << clientIp << ":" << ntohs(clientAddr.sin_port) << " | Client ID: " << (int)clientSocket << "\n";

        auto session = std::make_shared<ClientSession>(clientSocket);
        
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(session);
        }

        std::thread clientThread([this, session]() 
        {
            try 
            {
                session->handle();
            }
            catch (const std::exception& e) 
            {
                std::cerr << "[Server] Exception in ClientSession: " << e.what() << "\n";
            }

            removeClient(session);
        });

        clientThread.detach(); 
    }
}

void Server::removeClient(std::shared_ptr<ClientSession> client)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto thisClient = std::find(clients.begin(), clients.end(), client);
    if (thisClient != clients.end()) 
    {
        clients.erase(thisClient);
    }
}

void Server::stop()
{
    isRunning = false;
    if (serverSocket != INVALID_SOCKET) 
    {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& client : clients) 
    {
        client->disconnect();
    }
    clients.clear();
}

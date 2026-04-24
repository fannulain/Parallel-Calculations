#include "Server.h"
#include "ClientSession.h"
#include <iostream>

Server::Server(int port) :
    port(port),
    listenSocket(INVALID_SOCKET),
    isRunning(false)
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

    unsigned long mode = 1;
    if (ioctlsocket(listenSocket, FIONBIO, &mode) != 0) 
    {
        std::cerr << "ioctlsocket failed: " << WSAGetLastError() << std::endl;
        stop();
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
        fd_set readfds;
        fd_set writefds;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(listenSocket, &readfds);

        SOCKET maxSocket = listenSocket;

        for (auto& session : activeSessions) 
        {
            SOCKET socket = session->getSocket();
            if (session->getState() == ClientSession::State::READING) 
            {
                FD_SET(socket, &readfds);
            } 
            else if (session->getState() == ClientSession::State::SENDING) 
            {
                FD_SET(socket, &writefds);
            }

            if (socket > maxSocket) 
            {
                maxSocket = socket;
            }
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms

        int activity = select(static_cast<int>(maxSocket + 1), &readfds, &writefds, NULL, &timeout);

        if (activity == SOCKET_ERROR) 
        {
            if (isRunning) std::cerr << "select() failed: " << WSAGetLastError() << std::endl;
            break;
        }

        if (FD_ISSET(listenSocket, &readfds)) 
        {
            SOCKET clientSocket = accept(listenSocket, NULL, NULL);
            if (clientSocket != INVALID_SOCKET) 
            {
                std::cout << "Client connected." << std::endl;
                activeSessions.push_back(std::make_unique<ClientSession>(clientSocket, threadPool));
            }
        }

        for (auto currSession = activeSessions.begin(); currSession != activeSessions.end();) 
        {
            SOCKET socket = (*currSession)->getSocket();

            if ((*currSession)->getState() == ClientSession::State::READING && FD_ISSET(socket, &readfds)) 
            {
                (*currSession)->readData();
            }

            if ((*currSession)->getState() == ClientSession::State::SENDING && FD_ISSET(socket, &writefds)) 
            {
                (*currSession)->writeData();
            }

            if ((*currSession)->getState() != ClientSession::State::FINISHED 
                && (*currSession)->getState() != ClientSession::State::PROCESSING 
                && (*currSession)->isTimedOut(10)) 
            {
                (*currSession)->finish();
            }

            if ((*currSession)->getState() == ClientSession::State::FINISHED) 
            {
                currSession = activeSessions.erase(currSession);
            } 
            else 
            {
                currSession++;
            }
        }
    }
}

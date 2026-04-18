#ifndef SERVER_H
#define SERVER_H

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <memory>
#include "ClientHandler.h"

#pragma comment(lib, "Ws2_32.lib")

class Server
{
private:
    uint16_t port;
    SOCKET serverSocket;
    std::atomic<bool> isRunning;

    std::mutex clientsMutex;
    std::vector<std::shared_ptr<ClientSession>> clients;
    
    void acceptLoop();
    void removeClient(std::shared_ptr<ClientSession> client);

public:
    Server(uint16_t port);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void start();
    void stop();
};

#endif //SERVER_H
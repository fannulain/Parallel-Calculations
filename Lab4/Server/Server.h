#ifndef SERVER_H
#define SERVER_H

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include "ClientHandler.h"

#pragma comment(lib, "Ws2_32.lib")

class Server
{
private:
    uint16_t port;
    SOCKET serverSocket;
    std::atomic<bool> isRunning;

    void acceptLoop();

public:
    Server(uint16_t port);
    ~Server();

    void start();
    void stop();
};

#endif //SERVER_H
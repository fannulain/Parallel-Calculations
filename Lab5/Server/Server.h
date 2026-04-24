#ifndef SERVER_H
#define SERVER_H

#define FD_SETSIZE 2048
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <atomic>
#include <stdexcept>
#include <vector>
#include <memory>
#include "../ThreadPool/ThreadPool.h"

class ClientSession;

class Server 
{
public:
    Server(int port);
    ~Server();

    void start();
    void stop();

private:
    int port;
    SOCKET listenSocket;
    std::atomic<bool> isRunning;
    std::vector<std::unique_ptr<ClientSession>> activeSessions;
    ThreadPool threadPool;

    void acceptConnections();
};

#endif // SERVER_H

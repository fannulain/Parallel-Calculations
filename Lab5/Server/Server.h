#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <atomic>
#include <thread>
#include <stdexcept>

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

    void acceptConnections();
};

#endif // SERVER_H

#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#define FD_SETSIZE 2048
#include <winsock2.h>
#include <string>
#include <chrono>
#include <atomic>
#include "../ThreadPool/ThreadPool.h"

class ClientSession 
{
public:
    enum class State 
    {
        READING,
        PROCESSING,
        SENDING,
        FINISHED
    };

    ClientSession(SOCKET clientSocket, ThreadPool& pool);
    ~ClientSession();

    void readData();
    void writeData();
    
    State getState() const { return state.load(); }
    void setState(State newState) { state.store(newState); }
    void finish() { state.store(State::FINISHED); }
    SOCKET getSocket() const { return socket; }

    bool isTimedOut(int timeoutSeconds) const;
    void updateActivity();
    
    void processRequest();

private:
    SOCKET socket;
    std::atomic<State> state;
    ThreadPool& threadPool;

    std::string requestBuffer;
    std::string responseBuffer;
    size_t bytesSent;

    bool keepAlive;
    std::chrono::steady_clock::time_point lastActivity;
    
    void sendResponse(int statusCode, const std::string& statusMessage, const std::string& contentType, const std::string& body);
    std::string readFile(const std::string& filepath);
    void handleGetRequest(const std::string& path);
    void reset();
};

#endif // CLIENTSESSION_H

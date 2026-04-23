#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <winsock2.h>
#include <string>

class ClientSession 
{
public:
    ClientSession(SOCKET clientSocket);
    ~ClientSession();

    void process();

private:
    SOCKET socket;

    void sendResponse(int statusCode, const std::string& statusMessage, const std::string& contentType, const std::string& body);
    std::string readFile(const std::string& filepath);
    void handleGetRequest(const std::string& path);
};

#endif // CLIENTSESSION_H

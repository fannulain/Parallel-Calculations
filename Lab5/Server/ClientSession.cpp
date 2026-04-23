#include "ClientSession.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

ClientSession::ClientSession(SOCKET clientSocket) : socket(clientSocket) 
{}

ClientSession::~ClientSession() 
{
    if (socket != INVALID_SOCKET) 
    {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }
}

void ClientSession::process() 
{
    const int bufferSize = 4096;
    char buffer[bufferSize];

    int bytesReceived = recv(socket, buffer, bufferSize - 1, 0);
    if (bytesReceived > 0) 
    {
        buffer[bytesReceived] = '\0';
        std::string request(buffer);

        std::istringstream stream(request);
        std::string method, path, version;
        stream >> method >> path >> version;

        if (method == "GET") 
        {
            handleGetRequest(path);
        } 
        else 
        {
            sendResponse(405, "Method Not Allowed", "text/plain", "Method Not Allowed");
        }
    } 
    else if (bytesReceived == 0) 
    {
        std::cout << "Client disconnected connection during recv." << std::endl;
    } 
    else 
    {
        std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
    }
}

void ClientSession::handleGetRequest(const std::string& path) 
{
    std::string safePath = path;
    if (safePath == "" || safePath == "/") 
    {
        safePath = "/index.html";
    }

    std::string filepath = "pages" + safePath;

    std::string fileContent = readFile(filepath);
    
    if (!fileContent.empty()) 
    {
        std::string contentType = "text/html";
        if (filepath.find(".css") != std::string::npos) 
            contentType = "text/css";
        else if (filepath.find(".js") != std::string::npos) 
            contentType = "application/javascript";
        else if (filepath.find(".jpg") != std::string::npos || filepath.find(".jpeg") != std::string::npos) 
            contentType = "image/jpeg";
        else if (filepath.find(".png") != std::string::npos) 
            contentType = "image/png";

        sendResponse(200, "OK", contentType, fileContent);
    } 
    else 
    {
        sendResponse(404, "Not Found", "text/html", "<html><body><h1>404 Not Found</h1></body></html>");
    }
}

std::string ClientSession::readFile(const std::string& filepath) 
{
    if (filepath.find("..") != std::string::npos) 
    {
        return "";
    }

    std::ifstream file(filepath, std::ios::binary);
    if (file) 
    {
        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }
    return "";
}

void ClientSession::sendResponse(int statusCode, const std::string& statusMessage, const std::string& contentType, const std::string& body) 
{
    std::ostringstream responseStream;
    responseStream << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
    responseStream << "Content-Type: " << contentType << "\r\n";
    responseStream << "Content-Length: " << body.length() << "\r\n";
    responseStream << "Connection: close\r\n";
    responseStream << "\r\n";
    responseStream << body;

    std::string responseString = responseStream.str();
    int totalSent = 0;
    int dataSize = responseString.length();

    while (totalSent < dataSize) 
    {
        int bytesSent = send(socket, responseString.c_str() + totalSent, dataSize - totalSent, 0);
        if (bytesSent == SOCKET_ERROR) 
        {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            break;
        }
        totalSent += bytesSent;
    }
}

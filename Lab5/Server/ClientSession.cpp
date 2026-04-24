#include "ClientSession.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

ClientSession::ClientSession(SOCKET clientSocket, ThreadPool& pool) :
    socket(clientSocket),
    state(State::READING),
    bytesSent(0),
    keepAlive(false),
    threadPool(pool)
{
    unsigned long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) 
    {
        std::cerr << "ioctlsocket failed: " << WSAGetLastError() << std::endl;
    }
    updateActivity();
}

ClientSession::~ClientSession() 
{
    if (socket != INVALID_SOCKET) 
    {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }
}

void ClientSession::updateActivity()
{
    lastActivity = std::chrono::steady_clock::now();
}

bool ClientSession::isTimedOut(int timeoutSeconds) const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivity).count();
    return duration > timeoutSeconds;
}

void ClientSession::reset()
{
    requestBuffer.clear();
    responseBuffer.clear();
    bytesSent = 0;
    state = State::READING;
    updateActivity();
}

void ClientSession::readData() 
{
    const int bufferSize = 4096;
    char buffer[bufferSize];

    int bytesReceived = recv(socket, buffer, bufferSize - 1, 0);
    if (bytesReceived > 0) 
    {
        buffer[bytesReceived] = '\0';
        requestBuffer.append(buffer, bytesReceived);
        updateActivity();

        if (requestBuffer.find("\r\n\r\n") != std::string::npos) 
        {
            state.store(State::PROCESSING);
            threadPool.addTask([this]() {
                processRequest();
            });
        }
    } 
    else if (bytesReceived == 0) 
    {
        state = State::FINISHED;
    } 
    else 
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) 
        {
            state = State::FINISHED;
        }
    }
}

void ClientSession::processRequest()
{
    std::istringstream stream(requestBuffer);
    std::string method, path, version;
    stream >> method >> path >> version;

    std::string lowerBuffer = requestBuffer;
    std::transform(lowerBuffer.begin(), lowerBuffer.end(), lowerBuffer.begin(), ::tolower);

    if (version == "HTTP/1.1" && lowerBuffer.find("connection: close") == std::string::npos) 
    {
        keepAlive = true;
    }
    else if (lowerBuffer.find("connection: keep-alive") != std::string::npos)
    {
        keepAlive = true;
    }
    else 
    {
        keepAlive = false;
    }

    if (method == "GET") 
    {
        handleGetRequest(path);
    } 
    else 
    {
        sendResponse(405, "Method Not Allowed", "text/plain", "Method Not Allowed");
    }
    
    state = State::SENDING;
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
    
    if (keepAlive) 
    {
        responseStream << "Connection: keep-alive\r\n";
    } 
    else 
    {
        responseStream << "Connection: close\r\n";
    }
    
    responseStream << "\r\n";
    responseStream << body;

    responseBuffer = responseStream.str();
    bytesSent = 0;
}

void ClientSession::writeData() 
{
    if (bytesSent < responseBuffer.length()) 
    {
        int sent = send(socket, responseBuffer.c_str() + bytesSent, static_cast<int>(responseBuffer.length() - bytesSent), 0);
        if (sent > 0) 
        {
            bytesSent += sent;
            updateActivity();
        } 
        else if (sent == SOCKET_ERROR) 
        {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) 
            {
                state = State::FINISHED;
            }
        }
    }
    
    if (bytesSent >= responseBuffer.length()) 
    {
        if (keepAlive) 
        {
            reset();
        } 
        else 
        {
            state = State::FINISHED;
        }
    }
}

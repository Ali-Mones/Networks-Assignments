#pragma once

#include <netinet/in.h>
#include <string>

class Client
{
public:
    Client(const char* serverAddress, int serverPortNumber);
    ~Client();
    void RequestFile(std::string filepath);
    void ReceiveFile();
private:
    int m_Socket;
    sockaddr_in m_WelcomingServerAddress;
    sockaddr_in m_ReliableServerAddress;
};
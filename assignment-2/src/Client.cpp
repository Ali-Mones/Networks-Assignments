#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include "Client.h"

Client::Client(const char* serverAddress, int serverPortNumber)
{
    m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_Socket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(1);
    }

    m_WelcomingServerAddress.sin_family = AF_INET;
    m_WelcomingServerAddress.sin_port = htons(serverPortNumber);
    m_WelcomingServerAddress.sin_addr.s_addr = inet_addr(serverAddress);
}

Client::~Client()
{
    close(m_Socket);
}

void Client::RequestFile(std::string filepath)
{
    if (sendto(m_Socket, filepath.c_str(), filepath.size(), 0, (struct sockaddr*)&m_WelcomingServerAddress, sizeof(m_WelcomingServerAddress)) < 0) {
        std::cerr << "Failed to send data to server" << std::endl;
        exit(1);
    }
}

void Client::ReceiveFile()
{
    socklen_t reliableServerAddressLen = sizeof(m_ReliableServerAddress);
    // while (true)
    // {
        uint8_t buffer[512];
        memset(buffer, 0, sizeof(buffer));
        recvfrom(m_Socket, buffer, sizeof(buffer), 0, (sockaddr*)&m_ReliableServerAddress, &reliableServerAddressLen);
        printf("%s\n", buffer);
    // }
}

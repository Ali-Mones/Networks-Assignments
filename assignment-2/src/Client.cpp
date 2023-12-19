#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include "Client.h"
#include "Packets.h"

using namespace std;

Client::Client(const char* serverAddress, int serverPortNumber)
{
    m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_Socket < 0) {
        cerr << "Failed to create socket" << endl;
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

void Client::RequestFile(string filepath)
{
    if (sendto(m_Socket, filepath.c_str(), filepath.size(), 0, (struct sockaddr*)&m_WelcomingServerAddress, sizeof(m_WelcomingServerAddress)) < 0) {
        cerr << "Failed to send data to server" << endl;
        exit(1);
    }
}

void Client::ReceiveFile()
{
    socklen_t reliableServerAddressLen = sizeof(m_ReliableServerAddress);

    while (true)
    {
        Packet p;
        recvfrom(m_Socket, &p, sizeof(p), 0, (sockaddr*)&m_ReliableServerAddress, &reliableServerAddressLen);

        if (p.len == 0)
            break;

        cout << p.data << endl;
        cout << p.len << endl;

        AckPacket ack;
        sendto(m_Socket, &ack, sizeof(ack), 0, (sockaddr*)&m_ReliableServerAddress, reliableServerAddressLen);
    }
}

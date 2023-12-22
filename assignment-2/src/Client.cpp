#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <chrono>
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

    int ackno = 1;

    chrono::time_point timer = chrono::system_clock::now();

    while (true)
    {
        Packet p;
        recvfrom(m_Socket, &p, sizeof(p), 0, (sockaddr*)&m_ReliableServerAddress, &reliableServerAddressLen);
        // ackno++;

        if (p.len == 65535)
            break;

        if (p.len != 0)
        {
            cout << p.data << endl;
            cout << p.len << endl;
        }

        chrono::time_point now = chrono::system_clock::now();
        if (chrono::duration_cast<chrono::seconds>(now - timer) > chrono::seconds(6))
        {
            timer = now;
            ackno += p.len;
            AckPacket ack;
            ack.ackno = ackno;
            sendto(m_Socket, &ack, sizeof(ack), 0, (sockaddr*)&m_ReliableServerAddress, reliableServerAddressLen);
        }
    }
}

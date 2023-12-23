#pragma once

#include <netinet/in.h>
#include <string>
#include <queue>
#include <fstream>

#include "Packets.h"

using namespace std;

class PacketComparison
{
public:
    bool operator() (Packet* a, Packet* b)
    {
        return a->seqno > b->seqno;
    }
};

class Client
{
public:
    Client(const char* serverAddress, int serverPortNumber);
    ~Client();
    void RequestFile(std::string filepath);
    void ReceiveFile();
private:
    void SendAck(uint32_t ackno);
private:
    int m_Socket;
    sockaddr_in m_WelcomingServerAddress;
    sockaddr_in m_ReliableServerAddress;

    ofstream m_File;

    priority_queue<Packet*, vector<Packet*>, PacketComparison> m_Packets;
};

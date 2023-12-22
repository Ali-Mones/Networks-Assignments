#include <iostream>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <cmath>
#include <chrono>
#include <deque>

#include "Packets.h"
#include "Server.h"
using namespace std;

Server::Server(int welcoming_port)
{
    m_WelcomingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    int optval = 1;
    if (setsockopt(m_WelcomingSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed!");
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(welcoming_port);

    if (bind(m_WelcomingSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Server binding failed");
        exit(1);
    }

    cout << "Server ready on port " << welcoming_port << "!" << endl;
}

Server::~Server()
{
    close(m_WelcomingSocket);
}

void Server::Run()
{
    while (true)
    {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);

        uint8_t buffer[128];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recvfrom(m_WelcomingSocket, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddress, &clientAddressLen);

        if (bytesRead == -1)
        {
            perror("recvfrom failed!");
            exit(1);
        }
        
        pid_t childPid = fork();

        if (childPid == -1)
        {
            perror("Forking Failed");
            exit(1);
        }
        else if (childPid == 0)
        {
            // child process here
            string filepath = (char*)(&buffer);
            HandleRequest(clientAddress, filepath);
            cout << "Child exiting" << endl;
            exit(0);
        }
        else
        {
            // parent process here
            m_WaitingMutex.lock();
            if (!m_Waiting)
            {
                if (m_WaitingThread)
                {
                    m_WaitingThread->join();
                    delete m_WaitingThread;
                }

                m_Waiting = true;
                m_WaitingThread = new thread(&Server::AwaitChildren, this);
            }
            m_WaitingMutex.unlock();
        }
    }
}

void Server::AwaitChildren()
{
    while (wait(NULL) > 0);
    cout << "All children exited!" << endl;
    m_WaitingMutex.lock();
    m_Waiting = false;
    m_WaitingMutex.unlock();
}

void Server::HandleRequest(sockaddr_in clientAddress, string filepath)
{
    socklen_t clientAddressLen = sizeof(clientAddress);
    int reliableSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    ifstream file("files/" + filepath, ios::ate);
    size_t fileSize = file.tellg();
    file.seekg(0);

    int packetCount = ceil(fileSize / (float)sizeof(Packet::data));

    uint32_t nextSeqNum = 1;
    uint32_t sendBase = 1;

    deque<Packet*> unAckedPackets;

    chrono::time_point timer = chrono::system_clock::now();

    do
    {
        int remainingSize = fileSize - file.tellg();

        Packet* p = new Packet();
        p->len = min(remainingSize, (int)sizeof(p->data));
        p->seqno = nextSeqNum++;

        file.read((char*)&p->data, p->len);

        // add probability to fail
        sendto(reliableSocket, p, sizeof(Packet), 0, (sockaddr*)&clientAddress, clientAddressLen);

        if (p->len > 0)
            unAckedPackets.push_back(p);

        chrono::time_point now = chrono::system_clock::now();
        if (chrono::duration_cast<chrono::seconds>(now - timer) > chrono::seconds(4))
        {
            // handle timeout
            cout << "timed out!" << endl;
            cout << "send base = " << sendBase << endl;
            Packet* p = unAckedPackets.front();
            sendto(reliableSocket, p, sizeof(Packet), 0, (sockaddr*)&clientAddress, clientAddressLen);
            timer = chrono::system_clock::now();
        }

        AckPacket ack;
        recvfrom(reliableSocket, &ack, sizeof(ack), MSG_DONTWAIT, (sockaddr*)&clientAddress, &clientAddressLen);

        if (ack.ackno != 0)
        {
            // ack received
            int n = ack.ackno - sendBase;
            while (n--)
            {
                Packet* p = unAckedPackets.front();
                delete p;
                unAckedPackets.pop_front();
            }

            sendBase = max(sendBase, ack.ackno);
            if (nextSeqNum > sendBase)
            {
                // start timer
                timer = chrono::system_clock::now();
            }

            cout << "ackno = " << ack.ackno << endl;
            cout << "remaining packets = " << unAckedPackets.size() << endl;
        }
    } while (!unAckedPackets.empty());

    Packet endPacket;
    endPacket.len = -1;
    sendto(reliableSocket, &endPacket, sizeof(endPacket), 0, (sockaddr*)&clientAddress, clientAddressLen);

    close(reliableSocket);
}

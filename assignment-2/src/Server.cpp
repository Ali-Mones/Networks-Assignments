#include <iostream>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <cmath>

#include "Packets.h"
#include "Server.h"
using namespace std;

Server::Server(int welcoming_port)
{
    m_WelcomingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    int enable = 1;
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

    while (packetCount--)
    {
        int remainingSize = fileSize - file.tellg();
        cout << remainingSize << endl;

        Packet p;
        p.len = min(remainingSize, (int)sizeof(p.data));

        file.read((char*)&p.data, p.len);

        sendto(reliableSocket, &p, sizeof(p), 0, (sockaddr*)&clientAddress, clientAddressLen);

        AckPacket ack;
        recvfrom(reliableSocket, &ack, sizeof(ack), 0, (sockaddr*)&clientAddress, &clientAddressLen);
    }

    Packet endPacket;
    sendto(reliableSocket, &endPacket, sizeof(endPacket), 0, (sockaddr*)&clientAddress, clientAddressLen);

    close(reliableSocket);
}

#include <iostream>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
        perror("Server Binding Failed");
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

        uint8_t buffer[512];
        int bytesRead = recvfrom(m_WelcomingSocket, buffer, sizeof buffer, 0, (sockaddr*)&clientAddress, &clientAddressLen);

        if (bytesRead == -1)
        {
            perror("Receive From Failed!");
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
            CreateNewSocket(clientAddress);
            cout << "Child Exiting" << endl;
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
                    cout << "joining thread!" << endl;
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
    cout << "All children Exited!" << endl;
    m_WaitingMutex.lock();
    m_Waiting = false;
    m_WaitingMutex.unlock();
}

void Server::CreateNewSocket(sockaddr_in clientAddress)
{
    socklen_t clientAddressLen = sizeof(clientAddress);
    int reliableSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    string msg = "Hello from server!";
    sendto(reliableSocket, msg.c_str(), msg.size(), 0, (sockaddr*)&clientAddress, clientAddressLen);
    close(reliableSocket);
}

#pragma once

#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <string>

class Server
{
public:
    Server(int welcoming_port, float plp);
    ~Server();
    void Run();
private:
    void AwaitChildren();
    void HandleRequest(sockaddr_in clientAddress, std::string filepath);
private:
    int m_WelcomingSocket;
    bool m_Waiting;
    std::mutex m_WaitingMutex;
    std::thread* m_WaitingThread = nullptr;
    float m_PLP;
};

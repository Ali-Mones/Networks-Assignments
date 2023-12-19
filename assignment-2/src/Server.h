#include <netinet/in.h>
#include <thread>
#include <mutex>

class Server
{
public:
    Server(int welcoming_port);
    ~Server();
    void Run();
private:
    void AwaitChildren();
    void CreateNewSocket(sockaddr_in clientAddress);
private:
    int m_WelcomingSocket;
    bool m_Waiting;
    std::mutex m_WaitingMutex;
    std::thread* m_WaitingThread = nullptr;
};

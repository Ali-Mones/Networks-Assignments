#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>
using namespace std;

void create_new_socket(sockaddr_in client_address)
{
    socklen_t client_address_len = sizeof(client_address);
    int reliable_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    const char* msg = "Hello from server!";
    sendto(reliable_socket, msg, strlen(msg), 0, (sockaddr*)&client_address, client_address_len);
}

void wait_for_all_children(bool* waiting, mutex* waiting_mutex)
{
    while (wait(NULL) > 0);
    cout << "All children Exited!" << endl;
    waiting_mutex->lock();
    *waiting = false;
    waiting_mutex->unlock();
}

int main()
{
    int welcoming_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    if (bind(welcoming_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Server Binding Failed");
        exit(1);
    }

    // add lock on this variable
    bool waiting = false;
    mutex waiting_mutex;
    thread* waiting_thread = nullptr;

    while (true)
    {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        uint8_t buffer[512];
        int bytes_read = recvfrom(welcoming_socket, buffer, sizeof buffer, 0, (sockaddr*)&client_address, &client_address_len);

        if (bytes_read == -1)
        {
            perror("Receive From Failed!");
            exit(1);
        }

        pid_t child_pid = fork();

        if (child_pid == -1)
        {
            perror("Forking Failed");
            exit(1);
        }
        else if (child_pid == 0)
        {
            // child process here
            create_new_socket(client_address);
            cout << "Child Exiting" << endl;
            exit(0);
        }
        else
        {
            // parent process here
            waiting_mutex.lock();
            if (!waiting)
            {
                if (waiting_thread)
                {
                    waiting_thread->join();
                    delete waiting_thread;
                }

                waiting = true;
                waiting_thread = new thread(wait_for_all_children, &waiting, &waiting_mutex);
            }
            waiting_mutex.unlock();
        }
    }

    return 0;
}
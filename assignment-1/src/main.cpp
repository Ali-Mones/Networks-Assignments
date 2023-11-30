#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
using namespace std;

void* respond(void* arg)
{
    int clientSocket = *(int*)&arg;
    while (true)
    {
        uint8_t request[1024];
        int bytesRead = recv(clientSocket, request, sizeof(request), 0);
        if (bytesRead <= 0)
        {
            cerr << "Couldn't receive request from client!" << endl;
            break;
        }

        string response = "hello!";
        int bytesSent = send(clientSocket, response.c_str(), response.size() - 1, 0);
        if (bytesSent <= 0)
        {
            cerr << "Failed to send response to client!";
            break;
        }
    }

    cout << "Client disconnected!" << endl;

    close(clientSocket);
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        cerr << "Failed to create socket!" << endl;
        exit(1);
    }

    int enable = 1;
    int optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
    {
        cerr << "setsockopt(SO_REUSEADDR) failed!" << endl;
        exit(1);
    }

    cout << "Server socket: " << serverSocket << endl;

    sockaddr_in serverAddress = {0};
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
    {
        cerr << "Failed to bind server!" << endl;
        close(serverSocket);
        exit(1);
    }

    cout << "Server listening on port: 8080" << endl;

    if (listen(serverSocket, 5) == -1)
    {
        cerr << "Failed to listen on socket!" << endl;
        close(serverSocket);
        exit(1);
    }

    pthread_t threads[5] = {};
    int i = 0;
    while (true)
    {
        sockaddr_in clientAddress = {0};
        socklen_t clientAddressLen = sizeof(clientAddress);
        cout << "Awaiting connection!" << endl;
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            cerr << "Failed to accept request!" << endl;
            exit(1);
        }

        cout << "Client connected!" << endl;

        if (i >= 5)
        {
            cout << "out of bounds!" << endl;
            break;
        }

        pthread_create(&threads[i++], NULL, &respond, (void*)clientSocket);
    }
    
    close(serverSocket);
}
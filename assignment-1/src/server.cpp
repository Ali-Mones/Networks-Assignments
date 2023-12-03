#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
using namespace std;

#define BUFFER_SIZE 1024

char* openFile(const char* filePath)
{
    char* buffer = (char*)malloc(BUFFER_SIZE);
    ifstream file(filePath);
    cout << filesystem::is_regular_file(filePath) << endl;
    file.read(buffer, BUFFER_SIZE);
    file.close();

    return buffer;
}

void* respond(void* arg)
{
    int clientSocket = *(int*)&arg;
    while (true)
    {
        uint8_t request[BUFFER_SIZE];
        memset(request, 0, BUFFER_SIZE);
        int bytesRead = recv(clientSocket, request, BUFFER_SIZE, 0);

        if (bytesRead < 0)
        {
            cerr << "Couldn't receive request from client!" << endl;
            break;
        }

        if (bytesRead == 0)
            break;

        cout << request << endl;

        char* responseBody = openFile("hello.txt");
        char* responseHeaders = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n\r\n";

        char* response = (char*)malloc(20 * BUFFER_SIZE * BUFFER_SIZE);
        strcat(response, responseHeaders);
        strcat(response, responseBody);
        int responseLen = strlen(response);

        int bytesSent = send(clientSocket, response, responseLen, 0);
        free(responseBody);
        if (bytesSent <= 0)
            cerr << "Failed to send response to client!";
    }

    cout << "Client disconnected!" << endl;

    close(clientSocket);

    return NULL;
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

    vector<pthread_t> threads;
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

        pthread_t thread = {0};
        threads.push_back(thread);
        pthread_create(&threads[threads.size() - 1], NULL, &respond, (void*)clientSocket);
    }
}
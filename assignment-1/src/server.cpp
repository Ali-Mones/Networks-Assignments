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
#include <chrono>
using namespace std;

#define BUFFER_SIZE 1024

const static string textHTMLType = "text/html; charset=utf-8";
const static string blobType = "image/jpeg";

struct ConnectionInstance
{
    pthread_t tid;
    int socket;
    std::chrono::time_point<chrono::system_clock> lastUpdate;
};

string readBuffer(const string& filePath)
{
    if (!filesystem::is_regular_file(filePath))
        return "";

    ifstream file(filePath);
    stringstream buffer;
    buffer << file.rdbuf();

    cout << buffer.str() << endl;

    return buffer.str();
}

void writeBuffer(const string& filePath, const string& buffer)
{
    ofstream file(filePath);
    file << buffer;
}

void sendNotFound404(int clientSocket)
{
    string notFoundResponse = "";
    notFoundResponse += "HTTP/1.1 404 Not Found\r\n";
    notFoundResponse += "Content-Length: 0\r\n";
    notFoundResponse += "\r\n";

    if (send(clientSocket, notFoundResponse.c_str(), notFoundResponse.size(), 0) == -1)
        perror("Failed to send 404 Not Found response to client!");
}

void sendGetOK(int socket, const string& responseBody, const string& contentType)
{
    string okResponse;
    okResponse += "HTTP/1.1 200 OK\r\n";
    okResponse += "Connection: Keep-Alive\r\n";
    okResponse += "Content-Type: " + contentType + "\r\n";
    okResponse += "Content-Length: " + to_string(responseBody.size()) + "\r\n";
    okResponse += "\r\n";
    okResponse += responseBody;

    if (send(socket, okResponse.c_str(), okResponse.size(), 0) == -1)
        perror("Failed to send 200 OK response to client!");
}

void* handleTimeout(void* arg)
{
    vector<ConnectionInstance*>* connections = (vector<ConnectionInstance*>*)arg;
    chrono::seconds timeout = std::chrono::seconds(30);

    while (true)
    {
        chrono::time_point now = chrono::system_clock::now();
        for (ConnectionInstance*& connection : *connections)
        {
            int index = &connection - &(*connections)[0];
            chrono::seconds timeDifference = chrono::duration_cast<std::chrono::seconds>(now - connection->lastUpdate);
            if (timeDifference > timeout)
            {
                cout << "Client timed out!" << endl;
                pthread_cancel(connection->tid);
                connections->erase(index + connections->begin());
                close(connection->socket);
            }
        }
    }
}

void* handleRequest(void* arg)
{
    ConnectionInstance* connection = (ConnectionInstance*)arg;

    while (true)
    {
        string requestString;
        int bytesRead;
        uint8_t request[BUFFER_SIZE];
        do
        {
            memset(request, 0, BUFFER_SIZE);
            bytesRead = recv(connection->socket, request, BUFFER_SIZE, 0);
            connection->lastUpdate = chrono::system_clock::now();

            cout << request << endl;

            if (bytesRead < 0)
            {
                perror("Couldn't receive request from client!");
                break;
            }

            if (bytesRead == 0)
                break;

            requestString.insert(requestString.end(), &request[0], &request[BUFFER_SIZE]);
        } while (bytesRead == BUFFER_SIZE);

        filesystem::path path = "hello.txt";

        string responseBody = readBuffer(path);

        if (responseBody.size() == 0)
            sendNotFound404(connection->socket);
        else
            sendGetOK(connection->socket, responseBody, textHTMLType);
    }

    cout << "Client disconnected!" << endl;

    close(connection->socket);

    return nullptr;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        cerr << "A port number must be provided!" << endl;
        exit(1);
    }

    int portNumber = atoi(argv[1]);
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1)
    {
        perror("Failed to create socket!");
        exit(1);
    }

    int enable = 1;
    int optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed!");
        exit(1);
    }

    sockaddr_in serverAddress = {0};
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Failed to bind server!");
        close(serverSocket);
        exit(1);
    }

    cout << "Server listening on port: " << portNumber << endl;

    if (listen(serverSocket, 5) == -1)
    {
        perror("Failed to listen on socket!");
        close(serverSocket);
        exit(1);
    }

    vector<ConnectionInstance*> connections;
    pthread_t timeoutThread;
    pthread_create(&timeoutThread, NULL, &handleTimeout, &connections);

    while (true)
    {
        sockaddr_in clientAddress = {0};
        socklen_t clientAddressLen = sizeof(clientAddress);
        cout << "Awaiting connection!" << endl;
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            perror("Failed to accept request!");
            exit(1);
        }

        cout << "Client connected!" << endl;

        pthread_t thread;
        ConnectionInstance connectionInstance;
        connectionInstance.tid = thread;
        connectionInstance.lastUpdate = chrono::system_clock::now();
        connectionInstance.socket = clientSocket;

        connections.push_back(&connectionInstance);
        pthread_create(&connectionInstance.tid, NULL, &handleRequest, &connectionInstance);
    }
}

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

#include "fileHandling.h"
using namespace std;

#define BUFFER_SIZE 1024

const static string textHTMLType = "text/html; charset=utf-8";
const static string imageType = "image/jpeg";

struct ConnectionInstance
{
    pthread_t tid;
    int socket;
    std::chrono::time_point<chrono::system_clock> lastUpdate;
};

void parseRequest(const string& request, string* method, string* url, string* body)
{
    stringstream ss(request);
    ss >> *method >> *url;
    *url = url->substr(1);

    int bodyStart = request.find("\r\n\r\n") + 4;
    int bodyEnd = request.find('\0');

    *body = request.substr(bodyStart, bodyEnd - bodyStart);
    if (url->ends_with(".png") || url->ends_with(".jpeg") || url->ends_with(".jpg"))
        *body = request.substr(bodyStart);
}

string getContentType(const string& url)
{
    const static string textHTMLType = "text/html; charset=utf-8";
    const static string imageType = "image/jpeg";

    filesystem::path path = url;
    if (path.extension() == ".txt" || path.extension() == ".html")
        return textHTMLType;
    else if (path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg")
        return imageType;

    cerr << "(ERROR) Unsupported extension of type " << path.extension() << endl;
    exit(1);
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

void sendOK200(int socket, const string& responseBody, const string& contentType)
{
    string okResponse;
    okResponse += "HTTP/1.1 200 OK\r\n";
    okResponse += "Connection: keep-alive\r\n";
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
        string request;
        int bytesRead;
        uint8_t requestBuffer[BUFFER_SIZE];
        do
        {
            memset(requestBuffer, 0, BUFFER_SIZE);
            bytesRead = recv(connection->socket, requestBuffer, BUFFER_SIZE, 0);
            connection->lastUpdate = chrono::system_clock::now();

            if (bytesRead < 0)
            {
                perror("Couldn't receive request from client!");
            }

            if (bytesRead == 0)
            {
                cout << "Client disconnected!" << endl;
                close(connection->socket);
                return nullptr;
            }

            request.insert(request.end(), &requestBuffer[0], &requestBuffer[BUFFER_SIZE]);
        } while (bytesRead == BUFFER_SIZE);

        cout << request << endl;

        string method, url, body;
        parseRequest(request, &method, &url, &body);

        if (method == "GET")
        {
            string responseBody = readBuffer("server/" + url);
            if (responseBody.size() == 0)
                sendNotFound404(connection->socket);
            else
                sendOK200(connection->socket, responseBody, getContentType(url));
        }
        else if (method == "POST")
        {
            writeBuffer("server/" + url, body);
            sendOK200(connection->socket, "", "text/html");
        }
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

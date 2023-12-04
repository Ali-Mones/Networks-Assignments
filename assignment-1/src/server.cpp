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

const static string textHTMLType = "text/html; charset=utf-8";
const static string blobType = "image/jpeg";

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
    string response = "";
    response += "HTTP/1.1 404 Not Found\r\n";
    response += "Content-Length: 0\r\n";
    response += "\r\n";

    if (send(clientSocket, response.c_str(), response.size(), 0) == -1)
        perror("Failed to send 404 Not Found response to client!");
}

void sendGetOK(int socket, const string& responseBody, const string& contentType)
{
    string response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Connection: Keep-Alive\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + to_string(responseBody.size()) + "\r\n";
    response += "\r\n";
    response += responseBody;

    if (send(socket, response.c_str(), response.size(), 0) == -1)
        perror("Failed to send 200 OK response to client!");
}

void* respond(void* arg)
{
    int clientSocket = *(int*)&arg;
    while (true)
    {
        string requestString;
        int bytesRead;
        uint8_t request[BUFFER_SIZE];
        do
        {
            memset(request, 0, BUFFER_SIZE);
            bytesRead = recv(clientSocket, request, BUFFER_SIZE, 0);
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

        filesystem::path path = "index.html";
        string extension = path.extension();
        cout << extension << endl;

        string responseBody = readBuffer(path);

        if (responseBody.size() == 0)
            sendNotFound404(clientSocket);
        else
            sendGetOK(clientSocket, responseBody, textHTMLType);
    }

    cout << "Client disconnected!" << endl;

    close(clientSocket);

    return nullptr;
}

int main(int argc, char** argv)
{
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

    cout << "Server listening on port: 8080" << endl;

    if (listen(serverSocket, 5) == -1)
    {
        perror("Failed to listen on socket!");
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
            perror("Failed to accept request!");
            exit(1);
        }

        cout << "Client connected!" << endl;

        pthread_t thread = {0};
        threads.push_back(thread);
        pthread_create(&threads[threads.size() - 1], NULL, &respond, (void*)clientSocket);
    }
}
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>

#include "fileHandling.h"
using namespace std;

#define BUFFER_SIZE 1024

stringstream readInput(const string& filepath)
{
    ifstream file(filepath);
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer;
}

void sendGetRequest(int socket, const string& path)
{
    string request = "";
    request += "GET " + path + " HTTP/1.1\r\n";
    request += "\r\n";

    if (send(socket, request.c_str(), request.size(), 0) == -1)
        perror("(ERROR) Failed to send GET request!");
}

void sendPostRequest(int socket, const string& path, const string& body)
{
    string request = "";
    request += "POST " + path + " HTTP/1.1\r\n";
    request += "\r\n";
    request += body;

    cout << request << endl;

    if (send(socket, request.c_str(), request.size(), 0) == -1)
        perror("(ERROR) Failed to send POST request!");
}

string receiveResponse(int socket)
{
    string response;
    uint8_t responseBuffer[BUFFER_SIZE];
    int bytesRead;
    do
    {
        memset(responseBuffer, 0, BUFFER_SIZE);
        bytesRead = recv(socket, responseBuffer, BUFFER_SIZE, 0);

        if (bytesRead < 0)
        {
            perror("Couldn't receive response from server!");
        }

        if (bytesRead == 0)
        {
            cout << "Connection closed!" << endl;
            close(socket);
            return "";
        }

        response.insert(response.end(), &responseBuffer[0], &responseBuffer[BUFFER_SIZE]);
    } while (bytesRead == BUFFER_SIZE);

    return response;
}

void parseInput(string inputLine, string* method, string* path)
{
    stringstream ss(inputLine);
    ss >> *method >> *path;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        cerr << "(ERROR) You must provide both server ip and port numbers" << endl;
        exit(1);
    }

    const char* serverIp = argv[1];
    const char* serverPort = argv[2];

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
    {
        perror("Socket creation failed!");
        exit(1);
    }

    sockaddr_in serverAddress = {0};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(serverPort));

    if (inet_pton(AF_INET, serverIp, &serverAddress.sin_addr.s_addr) == -1)
    {
        perror("inet_pton() failed");
        exit(1);
    }

    if (connect(sock, (const sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Connection to server failed!");
        exit(1);
    }

<<<<<<< HEAD
    string url = "/hello.txt";
    string getRequest = "POST " + url + " HTTP/1.1\r\n\r\nfake youu";

    int bytesSent = send(sock, getRequest.c_str(), getRequest.size(), 0);
    if (bytesSent == -1 || bytesSent != getRequest.size())
        cerr << "Sending to server failed!" << endl;
=======
    stringstream input = readInput("client/input.txt");
>>>>>>> main

    string line;
    while (getline(input, line))
    {
        string method, path;
        parseInput(line, &method, &path);

        if (method == "client_get")
        {
            sendGetRequest(sock, path);
        }
        else
        {
            string body = readBuffer("client/" + path);
            sendPostRequest(sock, path, body);
        }
        
        string response = receiveResponse(sock);
        cout << response << endl;
    }

    close(sock);
}
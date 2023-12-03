#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <cstring>
using namespace std;

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
    {
        cerr << "Socket creation failed!" << endl;
        exit(1);
    }

    int serverPort = 8080;
    sockaddr_in serverAddress = {0};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    const char* serverIp = "127.0.0.1";
    if (inet_pton(AF_INET, serverIp, &serverAddress.sin_addr.s_addr) == -1)
    {
        cerr << "inet_pton() failed" << endl;
        exit(1);
    }

    if (connect(sock, (const sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
    {
        cerr << "Connection to server at " << serverIp << ":" << serverPort << " failed!" << endl;
        exit(1);
    }

    string url = "/hello.txt";
    string getRequest = "GET " + url + " HTTP/1.1\r\n\r\n";
    int bytesSent = send(sock, getRequest.c_str(), getRequest.size(), 0);
    if (bytesSent == -1 || bytesSent != getRequest.size())
        cerr << "Sending to server failed!" << endl;

    uint8_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);

    if (bytesReceived == 0)
        cerr << "Connection closed prematurely!" << endl;
    if (bytesReceived < 0)
        cerr << "Couldn't receive from server" << endl;

    if (bytesReceived > 0)
        cout << buffer << endl;

    close(sock);
}
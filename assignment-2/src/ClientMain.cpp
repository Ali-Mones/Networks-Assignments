#include "Client.h"
#include <string>

int main()
{
    Client client("127.0.0.1", 8082);
    client.RequestFile("hello.txt");
    client.ReceiveFile();
}
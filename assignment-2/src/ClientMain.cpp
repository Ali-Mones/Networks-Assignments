#include "Client.h"
#include <string>

int main()
{
    Client client("127.0.0.1", 8082);
    client.RequestFile("maro.txt");
    client.ReceiveFile();
}
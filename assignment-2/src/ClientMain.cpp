#include "Client.h"
#include <string>

int main()
{
    Client client("127.0.0.1", 8082);
    client.RequestFile("server_files/Credit Hours System List GSP-SSP 2020.pdf");
    client.ReceiveFile();
}
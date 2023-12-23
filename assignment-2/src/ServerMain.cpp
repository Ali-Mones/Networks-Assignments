#include "Server.h"

int main()
{
    Server server(8082, 0.3f);
    server.Run();
}

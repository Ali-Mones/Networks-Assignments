#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

int main() {
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Server address information
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);  // Replace with the actual server port
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Replace with the actual server IP address

    // Send data to the server
    const char* message = "Hello, server!";
    int messageLen = strlen(message);
    if (sendto(sockfd, message, messageLen, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to send data to server" << std::endl;
        return 1;
    }

    sockaddr_in reliable_address;
    socklen_t reliable_address_len = sizeof(reliable_address);

    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    recvfrom(sockfd, buffer, sizeof(buffer), 0, (sockaddr*)&reliable_address, &reliable_address_len);
    printf("%s\n", buffer);

    // Close the socket
    close(sockfd);

    return 0;
}

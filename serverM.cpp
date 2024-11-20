#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_M_UDP 24985
#define PORT_M_TCP 25985
#define PORT_A_UDP 21985
#define BUFFER_SIZE 1024
using namespace std;

int main() {
    int serverSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];
    socklen_t clientLen = sizeof(clientAddr);

    // Create TCP socket for communication with the client
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_M_TCP);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind and listen
    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSock);
        return -1;
    }
    listen(serverSock, 5);

    cout << "Server M: Main Server is up and running using UDP on port " << PORT_M_UDP << endl;

    while (true) {
        // Accept client connection
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSock < 0) {
            perror("Connection failed");
            continue;
        }

        memset(buffer, 0, BUFFER_SIZE);
        read(clientSock, buffer, BUFFER_SIZE);

        // Extract username and mask the password
        string credentials(buffer);
        size_t spacePos = credentials.find(" ");
        string username = credentials.substr(0, spacePos);
        string password = credentials.substr(spacePos + 1);

        // Print formatted message
        cout << "Server M has received username <" << username << "> and password ******" << std::endl;

        // Forward request to ServerA via UDP
        int udpSock;
        struct sockaddr_in serverAAddr;
        udpSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpSock < 0) {
            perror("UDP socket creation failed");
            close(clientSock);
            continue;
        }

        serverAAddr.sin_family = AF_INET;
        serverAAddr.sin_port = htons(PORT_A_UDP);
        serverAAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        sendto(udpSock, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAAddr, sizeof(serverAAddr));
        cout << "Server M has sent authentication request to Server A" << endl;
        // Receive response from ServerA
        memset(buffer, 0, BUFFER_SIZE);
        socklen_t serverALen = sizeof(serverAAddr);
        recvfrom(udpSock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAAddr, &serverALen);

        cout << "Server M received response from Server A using UDP over 24985: " << buffer << std::endl;

        // Send response back to the client
        write(clientSock, buffer, strlen(buffer));
        cout << "The main server has sent the response from server A to client using TCP over port 25985 " << endl;

        close(clientSock);
        close(udpSock);
    }

    close(serverSock);
    return 0;
}


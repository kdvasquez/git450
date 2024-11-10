#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>  // For inet_pton function
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_ADDRESS "127.0.0.1"  // Server IP address (localhost in this case)

using namespace std;

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 4) {
        cout << "Usage: ./client <username> <password> <port>" << endl;
        return -1;
    }

    string username = argv[1];  // First argument is username
    string password = argv[2];  // Second argument is password
    int port = stoi(argv[3]);   // Third argument is the server port

    cout << "The client is up and running." << endl;
    cout << "Sending credentials: " << username << " " << password << " to port " << port << endl;

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Step 1: Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Step 2: Set up server address struct
    server_addr.sin_family = AF_INET;       // Use IPv4
    server_addr.sin_port = htons(port);     // Set server port dynamically from input

    // Convert IP address from text to binary
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr) <= 0) {
        cerr << "Invalid address or address not supported" << endl;
        close(sockfd);
        return -1;
    }

    // Step 3: Send the username and password to the server
    string credentials = username + " " + password;
    int sent_bytes = sendto(sockfd, credentials.c_str(), credentials.size(), MSG_CONFIRM,
                            (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent_bytes < 0) {
        perror("Send failed");
        close(sockfd);
        return -1;
    }

    cout << "Message sent successfully." << endl;

    // Step 4: Wait for a response from the server (optional)
    socklen_t server_len = sizeof(server_addr);
    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &server_len);
    if (n < 0) {
        perror("Receive failed");
        close(sockfd);
        return -1;
    }

    // Null-terminate and display the response from the server
    buffer[n] = '\0';
    cout << "Server response: " << buffer << endl;

    // Step 5: Close the socket
    close(sockfd);

    return 0;
}

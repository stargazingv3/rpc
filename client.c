#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Function to tokenize a text file into words using proper delimiters
// ...

#define MAX_WORD_LEN 50 // Maximum word length

void tokenizeFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening input file");
        return;
    }

    char word[MAX_WORD_LEN];
    while (fscanf(file, "%s", word) == 1) {
        // Process each word
        // ...
    }

    fclose(file);
}

// Function to connect to the server and call remote functions
// ...
#define SERVER_IP "127.0.0.1" // Replace with the actual server IP
#define SERVER_PORT 8080      // Replace with the actual server port

void connectToServerAndCallRemoteFunctions(const char* word) {
    int clientSocket;
    struct sockaddr_in serverAddr;

    // Create a socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // Configure server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);

    // Resolve server IP and connect to the server
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(clientSocket);
        exit(1);
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr) < 0) {
        perror("Error connecting to server");
        close(clientSocket);
        exit(1);
    }

    // Implement code to call remote functions (e.g., encrypt) on the server
    // ...

    // Close the client socket
    close(clientSocket);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_host> <server_port> <input_file>\n", argv[0]);
        exit(1);
    }

    char* serverHost = argv[1];
    int serverPort = atoi(argv[2]);
    char* inputFile = argv[3];

    int clientSocket;
    struct sockaddr_in serverAddr;

    // Create a socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // Configure server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    // Resolve server host and connect to the server
    if (inet_pton(AF_INET, serverHost, &serverAddr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(clientSocket);
        exit(1);
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr) < 0) {
        perror("Error connecting to server");
        close(clientSocket);
        exit(1);
    }

    // Read and tokenize the input text file
    // ...

    // For each word, call the remote encrypt() function on the server
    // ...

    // Write the encrypted word and key to the output text file
    // ...

    // Close the client socket
    close(clientSocket);

    return 0;
}

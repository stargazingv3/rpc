#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "10.100.240.204"
#define SERVER_PORT 8080

// Define the RPC protocol operations
enum RPCOperation {
    RPC_ENCRYPT,
    RPC_DECRYPT,
};

// Structure for an RPC request
struct RPCRequest {
    enum RPCOperation operation;
    char text[1024];
    int key;
};

// Function to send an RPC request (client stub)
void send_rpc_request(int socket_fd, enum RPCOperation operation, const char* text) {
    struct RPCRequest request;
    request.operation = operation;
    strncpy(request.text, text, sizeof(request.text));
    send(socket_fd, &request, sizeof(request), 0);
}

// Function to receive an RPC response (client stub)
char* receive_rpc_response(int socket_fd) {
    char buffer[1024];
    ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        return strdup(buffer);
    } else {
		//printf("Buffer received: %s", strdup(buffer));
        perror("Error receiving response from server");
        return NULL;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2 || (argc - 1) % 3 != 2) {
        fprintf(stderr, "Usage: %s <command> <file> [<command> <file>] ...\n", argv[0]);
		printf("Number of args: %d", argc);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    struct sockaddr_in server_addr;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection to server failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Send RPC requests for each command and file
    for (int i = 1; i < argc; i += 3) {
        enum RPCOperation operation;
        if (strcmp(argv[i], "encrypt") == 0) {
            operation = RPC_ENCRYPT;
        } else if (strcmp(argv[i], "decrypt") == 0) {
            operation = RPC_DECRYPT;
        } else {
            fprintf(stderr, "Invalid command: %s\n", argv[i]);
            continue;
        }

        const char* file_name = argv[i + 1];

        // Read text from the specified file
        FILE* file = fopen(file_name, "r");
        if (file == NULL) {
            perror("Error opening file");
            continue;
        }

        char text[1024];
        if (fgets(text, sizeof(text), file) != NULL) {
            // Remove trailing newline if present
            size_t len = strlen(text);
            if (len > 0 && text[len - 1] == '\n') {
                text[len - 1] = '\0';
            }
            send_rpc_request(client_socket, operation, text);

            char* response = receive_rpc_response(client_socket);
            if (response) {
                printf("Server Response:\n%s\n", response);
                free(response);
            }
        } else {
            fprintf(stderr, "Error reading text from file: %s\n", file_name);
        }

        fclose(file);
    }

    // Cleanup and exit
    close(client_socket);
    return 0;
}

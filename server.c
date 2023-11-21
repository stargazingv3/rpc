#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 10

// Define the RPC protocol operations
enum RPCOperation {
    RPC_ENCRYPT,
    RPC_DECRYPT,
};

// Structure for an RPC request
struct RPCRequest {
    enum RPCOperation operation;
    char* word;
    int key;
};

struct RPCResponse {
    char* word;
    int key;
};

struct RPCResponse* create_RPC_Response(const char* text, int key){
    struct RPCResponse* response = malloc(sizeof(struct RPCResponse));
    response->word = strdup(text);
    if(key > 0){
        response->key = key;
    }
    //printf("Recieved: %s, Setting word to: %s\n", text, strdup(text));
    //printf("%s\n", request.word);
    return response;
}

void send_RPC_response(int socket, struct RPCResponse* response){
    send(socket, response, sizeof(response), 0);
}

char* server_encrypt(const char* text) {
    printf("Encrypting, returning %s\n", text);

    // Check if text is NULL
    if (text == NULL) {
        // Handle the case where text is NULL (optional)
        perror("Input text is NULL");
        return NULL;
    }

    // Attempt to duplicate the text
    char* copy = strdup(text);

    // Check if memory allocation was successful
    if (copy == NULL) {
        // Handle allocation failure
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    printf("Successful strdup, attempting return %s\n", copy);

    return copy;
}


// Function to decrypt the text (server stub)
char* server_decrypt(const char* text) {
    // Implement the server-side decryption logic here
    // This is just a placeholder
    return text;
}
	
// Function to handle an RPC request
struct RPCResponse* handle_rpc_request(const struct RPCRequest* request) {
    char* result = NULL;
    printf("Request info: %d for %s\n", request->operation, request->word);
    switch (request->operation) {
        case RPC_ENCRYPT:
			printf("Attempting Encryption\n");
            result = server_encrypt(request->word);
			printf("Encryption finished, received %s\n", result);
            struct RPCResponse* response = create_RPC_Response(result, -1);
            return response;    
            //break;
        case RPC_DECRYPT:
            result = server_decrypt(request->word);
            break;
        default:
			printf("Given empty RPCRequest\n");
            // Invalid operation
            break;
    }
    return NULL;
}

// Thread function to handle client requests
void* handle_client(void* client_socket) {
	
    int socket_fd = *(int*)client_socket;
    struct RPCRequest* request;
    struct RPCResponse* response;
    //char* response;
	
    ssize_t bytes_received = recv(socket_fd, &request, sizeof(request), 0);
    if (bytes_received > 0) {
    //while (recv(socket_fd, &request, sizeof(request), 0) > 0) {
	    printf("Received request for operation: %d, word: %s\n", request->operation, request->word);
        response = handle_rpc_request(&request);
		printf("Handling Client\n");
		
        if (response) {
			//printf(response);
            printf("Replying with word: %s, key: %s\n", response->word, response->key);
            send_RPC_response(socket_fd, response);
            //send(socket_fd, response, strlen(response), 0);
            //free(response);
        }
    }
	printf("Finished handling client\n");
    close(socket_fd);
    //free(client_socket);
    return NULL;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    pthread_t tid[MAX_CLIENTS];
    int clients_count = 0;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    // Listen for client connections
    if (listen(server_socket, 10) == 0) {
        printf("Server is listening on port %d...\n", PORT);
    } else {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    addr_size = sizeof(new_addr);

    while (1) {
        // Accept a new connection and create a thread for it
        new_socket = accept(server_socket, (struct sockaddr*)&new_addr, &addr_size);
        if (new_socket < 0) {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(new_addr.sin_addr), ntohs(new_addr.sin_port));

        // Create a thread to handle the client
        /*if (pthread_create(&tid[clients_count], NULL, handle_client, &new_socket) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }*/
	
	    handle_client(&new_socket);

        clients_count++;

        // If there are too many clients, close the new socket
        if (clients_count >= MAX_CLIENTS) {
            printf("Maximum number of clients reached. Closing connection...\n");
            close(new_socket);
        }
    }

    // Close the server socket (never reached in this loop)
    close(server_socket);

    return 0;
}


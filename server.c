#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define MAX_SHIFT 9
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_words_processed = 0;

// Define the RPC protocol operations
enum RPCOperation {
    RPC_ENCRYPT,
    RPC_DECRYPT,
    RPC_CLOSE,
};

// Structure for an RPC request
struct RPCRequest {
    enum RPCOperation operation;
    char word[1024];
    int key;
};

struct RPCResponse {
    //char* word;
    char word[1024];
    int key;
};

struct Encrypted {
    char* word;
    int key;
};

struct RPCResponse* create_RPC_Response(struct Encrypted* result){
    struct RPCResponse* response = malloc(sizeof(struct RPCResponse));
    //response->word = strdup(text);
    strncpy(response->word, result->word, sizeof(response->word) - 1);
    response->word[sizeof(response->word) - 1] = '\0';
    response->key = result->key;
    //printf("Recieved: %s, Setting word to: %s\n", text, strdup(text));
    //printf("%s\n", request.word);
    return response;
}

void send_RPC_response(int socket, struct RPCResponse* response){
    //send(socket, response, sizeof(*response), 0);
    if (send(socket, response, sizeof(*response), 0) == -1) {
        perror("send failed");
        // Handle error
    }
}

// Function to generate a random number between 1 and 25
int generate_random_key() {
    return (rand() % MAX_SHIFT) + 1;
}

// Function to apply Caesar cipher encryption
char* caesar_cipher_encrypt(const char* text, int key) {
    int length = strlen(text);
    char* encrypted = (char*)malloc(length + 1);
    
    if (encrypted == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    int i;

    for (i = 0; i < length; i++) {
        char c = text[i];

        if (isalpha(c)) {
            char base = islower(c) ? 'a' : 'A';
            encrypted[i] = (c - base + key) % 26 + base;
        } else {
            encrypted[i] = c; // Non-alphabetic characters remain unchanged
        }
    }

    encrypted[length] = '\0';
    return encrypted;
}

// Function to apply Caesar cipher decryption
char* caesar_cipher_decrypt(const char* text, int key) {
    return caesar_cipher_encrypt(text, 26 - key); // Decryption is the reverse of encryption
}

// Encode the text using Caesar cipher
struct Encrypted* server_encrypt(const char* text) {
    int key = generate_random_key();
    printf("Encrypting, returning %s\n", text);

    // Check if text is NULL
    if (text == NULL) {
        // Handle the case where text is NULL (optional)
        perror("Input text is NULL");
        return NULL;
    }

    // Encrypt the text using Caesar cipher
    char* encrypted_text = caesar_cipher_encrypt(text, key);

    // Create an Encrypted struct to store the result
    struct Encrypted* ret = (struct Encrypted*)malloc(sizeof(struct Encrypted));
    ret->word = encrypted_text;
    ret->key = key;

    return ret;
}

// Decode the text using Caesar cipher
struct Encrypted* server_decrypt(const char* text, int key) {
    // Check if text is NULL
    if (text == NULL) {
        // Handle the case where text is NULL (optional)
        perror("Input text is NULL");
        return NULL;
    }

    // Decrypt the text using Caesar cipher
    char* decrypted_text = caesar_cipher_decrypt(text, key);

    // Create an Encrypted struct to store the result
    struct Encrypted* ret = (struct Encrypted*)malloc(sizeof(struct Encrypted));
    ret->word = decrypted_text;
    ret->key = key;

    return ret;
}
	
// Function to handle an RPC request
struct RPCResponse* handle_rpc_request(const struct RPCRequest* request) {
    struct Encrypted* result = malloc(sizeof(struct Encrypted));
    printf("Request info: %d for %s\n", request->operation, request->word);
    struct RPCResponse* response;
    switch (request->operation) {
        case RPC_ENCRYPT:
			printf("Attempting Encryption\n");
            result = server_encrypt(request->word);
			printf("Encryption finished, received %s\n", result->word);
            response = create_RPC_Response(result);
            return response;    
            //break;
        case RPC_DECRYPT:
            printf("Decrypting with key: %d\n", request->key);
            result = server_decrypt(request->word, request->key);
            response = create_RPC_Response(result);
            return response;
            //break;
        case RPC_CLOSE:
            /*result->key = -1;
            result->word = "NULL";
            response = create_RPC_Response(result);
            return response;*/
            response = malloc(sizeof(struct RPCResponse));
            strcpy(response->word, "NULL");
            response->key = -1;
            return response;
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
    struct timeval tv;
    int client_words_processed = 0;
    /*tv.tv_sec = 5;  // Set the timeout in seconds
    tv.tv_usec = 0; // ...and microseconds (0 in this case)

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        perror("setsockopt failed");
        // Handle the error. You might want to close the socket or exit the program.
    }*/
    struct RPCRequest request;
    struct RPCResponse* response;
    //char* response;
	
    //ssize_t bytes_received = recv(socket_fd, &request, sizeof(request), 0);
    //if (bytes_received > 0) {
    ssize_t bytes_received;
    while ((bytes_received = recv(socket_fd, &request, sizeof(request), 0)) > 0) {
	    printf("Received request for operation: %d, word: %s\n", request.operation, request.word);
        response = handle_rpc_request(&request);
		printf("Handling Client\n");
		
        if (response) {
            //printf("\n\nResponse Received with key:%d\n\n", response->key);
            if(response->key == -1){
                printf("\n\nReceived request to close connection\n\n");
                //close(socket_fd);
                printf("Finished handling client\n");
                pthread_mutex_lock(&count_mutex);
                total_words_processed += client_words_processed;
                printf("Total words processed: %d\n", total_words_processed);
                pthread_mutex_unlock(&count_mutex);
                close(socket_fd);
                return;
            }
			//printf(response);
            printf("Replying with word: %s, key: %d\n", response->word, response->key);
            send_RPC_response(socket_fd, response);
            //pthread_mutex_lock(&count_mutex);
            //total_words_processed++;
            //printf("Total words processed: %d\n", total_words_processed);
            //pthread_mutex_unlock(&count_mutex);
            client_words_processed++;
            //send(socket_fd, response, strlen(response), 0);
            //free(response);
        }
        else{
            printf("No response received\n");
        }
    }
    if (bytes_received < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("recv timed out\n");
            // Handle timeout-specific logic
        } else {
            perror("recv error");
            // Handle other errors
        }
    }   
	printf("Finished handling client\n");
    pthread_mutex_lock(&count_mutex);
    total_words_processed += client_words_processed;
    printf("Total words processed: %d\n", total_words_processed);
    pthread_mutex_unlock(&count_mutex);
    /*pthread_mutex_lock(&count_mutex);
    printf("Total words processed: %d\n", total_words_processed);
    pthread_mutex_unlock(&count_mutex);*/
    close(socket_fd);
    //free(client_socket);
    return;
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
    srand(time(NULL)); // Seed the random number generator

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

        printf("Connection accepted from %s:%d, creating a thread\n", inet_ntoa(new_addr.sin_addr), ntohs(new_addr.sin_port));

	    //handle_client(&new_socket);
        // Create a thread to handle the client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void*)&new_socket) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }

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


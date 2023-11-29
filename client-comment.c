#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

// Global variables
char SERVER_IP[16];  // Assuming IPv4 address, adjust the size accordingly
int SERVER_PORT;
#define MAGIC_NUMBER "aa231fd13)@!!@!%asdj3jfddjlmbt"
pthread_mutex_t total_words_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_words_processed = 0;

// Define the RPC protocol operations
enum RPCOperation {
    RPC_ENCRYPT,
    RPC_DECRYPT,
    RPC_CLOSE,
};

struct RPCRequest {
    enum RPCOperation operation;
    char word[1024];
    int key;
};

struct RPCResponse {
    char word[1024];
    int key;
};

typedef struct Node {
    char* word;
    struct Node* next;
} Node;

struct LineData {
    char* content;
    int key;
};

// Function to create a new node
Node* createNode(const char* word) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        perror("Failed to allocate memory for node");
        exit(1);
    }
    newNode->word = strdup(word); // Duplicate the word
    newNode->next = NULL;
    return newNode;
}

// Function to append a node to the list
void appendNode(Node** head, const char* word) {
    Node* newNode = createNode(word);
    if (*head == NULL) {
        // If the list is empty, the new node becomes the head
        *head = newNode;
    } else {
        // Otherwise, traverse the list to the end and append the new node
        Node* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

// Function to free the linked list
void freeList(Node* head) {
    Node* tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp->word);
        free(tmp);
    }
}

// Function to create an RPC request and send it to the server
void create_RPC_Request(enum RPCOperation operation, const char* word, int key, int socket) {
    // Allocate memory for the RPCRequest structure
    struct RPCRequest* request = malloc(sizeof(struct RPCRequest));
    if (!request) {
        perror("Failed to allocate memory for RPC request");
        exit(1);
    }

    // Populate the RPCRequest structure with the provided parameters
    request->operation = operation;
    if (word == NULL) {
        strncpy(request->word, "NULL", sizeof(request->word) - 1);
        request->word[sizeof(request->word) - 1] = '\0';
        request->key = -1;
    } else {
        strncpy(request->word, word, sizeof(request->word) - 1);
        request->word[sizeof(request->word) - 1] = '\0';
    }
    if (key > 0) {
        request->key = key;
    }

    // Send the RPC request to the server
    if (send(socket, request, sizeof(*request), 0) == -1) {
        perror("send failed");
        // Handle error
    }

    // Free the allocated memory for the RPCRequest structure
    free(request);
}

// Function to establish a connection to the server
int connect_to_server() {
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

    return client_socket;
}

// Function to receive an RPC response (client stub)
struct RPCResponse* receive_RPC_response(int socket_fd) {
    // Allocate memory for the RPCResponse structure
    struct RPCResponse* response = malloc(sizeof(struct RPCResponse));
    if (!response) {
        perror("Failed to allocate memory for RPC response");
        exit(1);
    }

    // Receive the RPC response from the server
    ssize_t bytes_received = recv(socket_fd, response, sizeof(struct RPCResponse), 0);
    if (bytes_received > 0) {
        // Check if the response is valid
        if (response) {
            return response;
        } else {
            perror("Error receiving response from server");
            return NULL;
        }
    }

    // Free the allocated memory for the RPCResponse structure
    free(response);
    return NULL;
}

// Function to handle server decryption
void* handle_server_decryption(char* word, FILE* file) {
    if (file == NULL) {
        perror("Invalid file pointer");
        return NULL;
    }

    // Write the decrypted word to the file
    fprintf(file, "%s ", word);
    return NULL;
}

// Function to handle server encryption
void* handle_server_encryption(char* word, int key, FILE* file) {
    if (file == NULL) {
        fprintf(stderr, "Invalid file pointer\n");
        return NULL;
    }

    // Write the encrypted word and key to the file
    fprintf(file, "{%s : %d},", word, key);
    return NULL;
}

// Function to free LineData structure
void freeLineData(struct LineData* data) {
    free(data->content);
    free(data);
}

// Function to parse a line of data
struct LineData* parseLine(const char* line) {
    // Allocate memory for the LineData structure
    struct LineData* data = (struct LineData*)malloc(sizeof(struct LineData));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    // Allocate memory for the content string
    data->content = strdup(line);
    if (data->content == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        free(data);
        return NULL;
    }

    // Exclude the first and last characters from content
    size_t lineLength = strlen(line);
    if (lineLength >= 2) {
        data->content = strdup(line + 1);
        data->content[lineLength - 2] = '\0';  // Null-terminate excluding the last character
    } else {
        fprintf(stderr, "Invalid format: %s\n", line);
        freeLineData(data);
        return NULL;
    }

    // Parse the key from the content
    char* keyStr = strstr(data->content, " : ");
    if (keyStr == NULL) {
        fprintf(stderr, "Invalid format: %s\n", line);
        freeLineData(data);
        return NULL;
    }

    *keyStr = '\0';  // Null-terminate the content
    keyStr += 3;     // Move past " : "
    if (sscanf(keyStr, "%d", &(data->key)) != 1) {
        fprintf(stderr, "Error parsing key: %s\n", line);
        freeLineData(data);
        return NULL;
    }

    return data;
}

// Function to decrypt a file
void* decryptFile(const char* file_name) {
    char line[256];
    int words_processed = 0;

    // Open the input file for reading
    FILE* file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("Failed to open input file");
        return NULL;
    }

    // Ignore the first line
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return NULL;
    }

    // Read and process subsequent lines
    enum RPCOperation operation = RPC_DECRYPT;
    int socket = connect_to_server();
    size_t file_size = strlen(file_name) + strlen("_dec") + 1;
    char* name = (char*)malloc(file_size);
    if (name == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        return NULL;
    }

    // Create the output file for decrypted content
    snprintf(name, file_size, "%s_dec", file_name);
    FILE* dec_file = fopen(name, "w");

    if (dec_file == NULL) {
        perror("Failed to open decryption output file");
        free(name);
        fclose(file);
        return NULL;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        // Assuming the entries are separated by commas within curly braces
        char* token = strtok(line, ",");
        
        while (token != NULL) {
            // Process each entry using parseLine
            struct LineData* data = parseLine(token);
            if (data != NULL) {
                // Create RPC request and send it to the server
                create_RPC_Request(operation, data->content, data->key, socket);
                // Receive and process RPC response
                struct RPCResponse* response = receive_RPC_response(socket);
                if (response) {
                    // Handle the decrypted word
                    handle_server_decryption(response->word, dec_file);
                    free(response);
                    words_processed++;
                }
                // Free memory for LineData
                freeLineData(data);
            } else {
                // Parsing failed for an entry
                printf("Error parsing entry: %s\n", token);
                fclose(dec_file);
                fclose(file);
                free(name);
                return NULL;
            }

            // Move to the next entry
            token = strtok(NULL, ",");
        }
    }

    // Close the connection to the server
    operation = RPC_CLOSE;
    create_RPC_Request(operation, line, 0, socket); 
    close(socket);

    // Free memory for the output file name
    free(name);

    // Print the number of words processed and update the global count
    printf("Words processed in %s: %d\n", file_name, words_processed);
    pthread_mutex_lock(&total_words_mutex);
    total_words_processed += words_processed;
    pthread_mutex_unlock(&total_words_mutex);

    // Add a newline at the end of the decrypted file
    fprintf(dec_file, "\n");

    // Close the output file
    fclose(dec_file);
    fclose(file);
    return NULL;
}

// Function to encrypt a file
void* encryptFile(const char* file_name) {
    char command[256];
    FILE *fp;
    char buffer[1024]; // Adjust buffer size as needed
    int words_processed = 0;

    // Run a command to obtain word count from the input file
    snprintf(command, sizeof(command), "./word-count %s", file_name);

    // Execute the command and open a pipe to read the output
    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Failed to run command");
        exit(1);
    }

    // Read the output of the command into a linked list
    Node* head = NULL;
    while (fgets(buffer, sizeof(buffer) - 1, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        appendNode(&head, buffer);
    }

    // Close the pipe
    pclose(fp);

    // Allocate memory for the output file name
    size_t file_size = strlen(file_name) + strlen("_enc") + 1;
    char* name = (char*)malloc(file_size);
    if (name == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    // Create the output file for encrypted content
    snprintf(name, file_size, "%s_enc", file_name);
    FILE* enc_file = fopen(name, "w");
    free(name);
    if (enc_file == NULL) {
        fprintf(stderr, "Failed to open encryption output file\n");
        return NULL;
    }

    // Write a magic number to identify encrypted files
    char entry[256];
    snprintf(entry, sizeof(entry), "%s\n", MAGIC_NUMBER);
    fprintf(enc_file, "%s", entry);

    // Establish a connection to the server
    int socket = connect_to_server();

    // Process each word in the linked list
    Node* current = head;
    enum RPCOperation operation = RPC_ENCRYPT;
    while (current->next) {
        // Create RPC request and send it to the server
        create_RPC_Request(operation, current->word, -1, socket);
        // Receive and process RPC response
        struct RPCResponse* response = receive_RPC_response(socket);
        if (response) {
            // Handle the encrypted word
            handle_server_encryption(response->word, response->key, enc_file);
            free(response);
            words_processed++;
        }
        current = current->next;
    }

    // Process the last word in the linked list
    create_RPC_Request(operation, current->word, -1, socket);
    struct RPCResponse* response = receive_RPC_response(socket);
    if (response) {
        // Handle the encrypted word
        handle_server_encryption(response->word, response->key, enc_file);
        free(response);
        words_processed++;
    }

    // Free memory for the linked list
    freeList(head);

    // Close the connection to the server
    operation = RPC_CLOSE;
    create_RPC_Request(operation, "close", -1, socket);
    struct RPCResponse* close_response = receive_RPC_response(socket);
    if (close_response) {
        free(close_response);
    }
    close(socket);

    // Print the number of words processed and update the global count
    printf("Words processed in %s: %d\n", file_name, words_processed);
    pthread_mutex_lock(&total_words_mutex);
    total_words_processed += words_processed;
    pthread_mutex_unlock(&total_words_mutex);

    // Close the output file
    fclose(enc_file); 
    return NULL;
}

// Function to process a file (main worker function for threads)
void* process_file(void* arg) {
    // Cast the argument to the filename
    const char* file_name = (const char*)arg;

    // Open the file for reading
    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    // Read the magic number from the beginning of the file
    size_t len = strlen(MAGIC_NUMBER);
    char buf[len + 1]; 
    if (fread(buf, 1, len, file) == len) {
        buf[len] = '\0';
        fclose(file);

        // Check if the magic number matches the predefined constant
        if (strcmp(buf, MAGIC_NUMBER) == 0) {
            // Magic number indicates decryption, call the decryptFile function
            decryptFile(file_name);
        } else {
            // Magic number does not match, indicating encryption, call the encryptFile function
            encryptFile(file_name);
        }
    }   
    else {
        // Unable to read the magic number, default to encryption
        encryptFile(file_name);
    }

    return NULL;
}

// Main function
int main(int argc, char* argv[]) {
    // Check if the command-line arguments are sufficient
    if (argc < 4) {
        fprintf(stderr, "Usage: %s IP PORT file1 file2 ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Extract server IP and port from command-line arguments
    const char* ip = argv[1];
    const char* server_port_str = argv[2];
    const int port = atoi(server_port_str);

    // Validate the port number
    if (port <= 0) {
        fprintf(stderr, "Invalid port number\n");
        return EXIT_FAILURE;
    }

    // Set global variables with server information
    strncpy(SERVER_IP, ip, sizeof(SERVER_IP) - 1);
    SERVER_IP[sizeof(SERVER_IP) - 1] = '\0';
    SERVER_PORT = port;

    // Create an array of thread identifiers for parallel file processing
    pthread_t threads[argc - 3];

    int i;

    // Iterate through the input files and create threads for parallel processing
    for (i = 3; i < argc; i++) {
        if (pthread_create(&threads[i - 3], NULL, process_file, (void*)argv[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Wait for all threads to finish
    for (i = 0; i < argc - 3; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

    // Print the total words processed by all threads
    printf("Total words processed by all threads: %d\n", total_words_processed);

    return 0;
}

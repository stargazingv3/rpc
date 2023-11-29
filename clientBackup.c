#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

char SERVER_IP[16];  // Assumes IPv4 address
int SERVER_PORT;
#define MAGIC_NUMBER "aa231fd13)@!!@!%asdj3jfddjlmbt" // Used to identify encrypted files
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

//Used to create a list of words to encrypt
typedef struct Node {
    char* word;
    struct Node* next;
} Node;

//Used to store the data and key to decrypt
struct LineData {
    char* content;
    int key;
};

Node* createNode(const char* word) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        perror("Failed to allocate memory for node");
        exit(1);
    }
    newNode->word = strdup(word);
    newNode->next = NULL;
    return newNode;
}

void appendNode(Node** head, const char* word) {
    Node* newNode = createNode(word);
    if (*head == NULL) {
        *head = newNode;
    } else {
        Node* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

void freeList(Node* head) {
    Node* tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp->word);
        free(tmp);
    }
}

//Creates and sends an RPC request given the operation to perform, and necessary parameters
void create_RPC_Request(enum RPCOperation operation, const char* word, int key, int socket){
    struct RPCRequest* request = malloc(sizeof(struct RPCRequest));
    request->operation = operation;

    if(word == NULL){ //Receiving "NULL" is requesting the server to close the connection
        strncpy(request->word, "NULL", sizeof(request->word) - 1);
        request->word[sizeof(request->word) - 1] = '\0';
        request->key = -1; // Key of -1 is used to tell the server to close the connection
    }
    else{
        strncpy(request->word, word, sizeof(request->word) - 1);
        request->word[sizeof(request->word) - 1] = '\0';
    }
    if(key > 0){
        request->key = key;
    }
    if (send(socket, request, sizeof(*request), 0) == -1) {
        perror("send failed");
    }
    free(request);
}

//Initializes connection to server and sends back a socket to use
int connect_to_server(){
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
struct RPCResponse* receive_RPC_response(int socket) {
    struct RPCResponse* response = malloc(sizeof(struct RPCResponse));
    int socket_fd = socket;
    struct timeval tv;
    //int client_words_processed = 0;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        perror("setsockopt failed");
        close(socket_fd);
        return NULL;
    }
    ssize_t bytes_received = recv(socket_fd, response, sizeof(struct RPCResponse), 0);
    if (bytes_received > 0) {
        if(response){
            return response;
        } else {
            perror("Error receiving response from server");
            return NULL;
        }
    }
    if (bytes_received < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("Lost packet from one of the threads, please quit and re-run the client\n\n");
            // Handle timeout-specific logic
        } else {
            perror("recv error");
            // Handle other errors
        }
    }  
    free(response);
    return NULL;
}

//Function to process the response of a server for decryption
//Writes the server's output to the specified file.
void* handle_server_decryption(char* word, FILE* file){
//int handle_server_decryption(int socket, FILE* file){
    
    if (file == NULL) {
        perror("Invalid file pointer");
        return 0;
    }

    /*struct RPCResponse* response = receive_RPC_response(socket);
    if(response){
        printf("Thread %lu Writing to file %p\n", pthread_self(), file);*/
        fprintf(file, "%s ", word);
       /* return 1;
    }
    return 0;*/
    return NULL;
}

//Function to process the response of a server for encryption
//Writes the server's output to the specified file.
//void* handle_server_encryption(char* word, int key, FILE* file){
int handle_server_encryption(int socket, FILE * file){
    if (file == NULL) {
        fprintf(stderr, "Invalid file pointer\n");
        return 0;
    }

    struct RPCResponse* response = receive_RPC_response(socket);
    if(response){
        printf("Thread %lu Writing to file %p\n", pthread_self(), file);
        fprintf(file, "{%s : %d},", response->word, response->key);
        return 1;
    }

    //fprintf(file, "{%s : %d},", word, key); // Write directly to the passed file
    return 0;
}

void freeLineData(struct LineData* data) {
    free(data->content);
    free(data);
}

struct LineData* parseLine(const char* line) {
    struct LineData* data = (struct LineData*)malloc(sizeof(struct LineData));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    // Assuming the content can be any length
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

void* decryptFile(const char* file_name) {
    char line[256];
    int words_processed = 0;

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
            //printf("Sending token: %s\n", token);
            struct LineData* data = parseLine(token);
            if (data != NULL) {
                create_RPC_Request(operation, data->content, data->key, socket);
                /*if(handle_server_decryption(socket, dec_file)){
                    words_processed++;
                }
                else{
                    printf("Error receiving server response\n");
                }*/
                struct RPCResponse* response = receive_RPC_response(socket);
                if (response) {
                    handle_server_decryption(response->word, dec_file);
                    free(response);
                    words_processed++;
                }
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

    operation = RPC_CLOSE;
    create_RPC_Request(operation, line, 0, socket); 
    close(socket);
    free(name);
    printf("Words processed in %s: %d\n", file_name, words_processed);
    pthread_mutex_lock(&total_words_mutex);
    total_words_processed += words_processed;
    pthread_mutex_unlock(&total_words_mutex);
    fprintf(dec_file, "\n");
    fclose(dec_file);
    fclose(file);
    return NULL;
}

void* encryptFile(const char* file_name){
    //printf("Encrypting file %s\n", file_name);
    char command[256];
    FILE *fp;
    char buffer[1024]; // Adjust buffer size as needed
    int words_processed = 0;

    //printf("Starting command\n");
    snprintf(command, sizeof(command), "./word-count %s", file_name);
   // printf("Finished running command\n");

    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Failed to run command");
        exit(1);
    }

    Node* head = NULL;

    while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        appendNode(&head, buffer);
    }

    pclose(fp);

    size_t file_size = strlen(file_name) + strlen("_enc") + 1;
    char* name = (char*)malloc(file_size);
    if(name == NULL){
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    snprintf(name, file_size, "%s_enc", file_name);
    FILE* enc_file = fopen(name, "w");
    free(name);
    if (enc_file == NULL) {
        fprintf(stderr, "Failed to open encryption output file\n");
        return NULL;
    }
    
    char entry[256];
    snprintf(entry, sizeof(entry), "%s\n", MAGIC_NUMBER);

    // Write the formatted data to the file
    fprintf(enc_file, "%s", entry);
    int socket = connect_to_server();

    Node* current = head;
    current = head;
    enum RPCOperation operation = RPC_ENCRYPT;
    while(current->next){
        //printf("Doing operation: %d\n", operation);
        create_RPC_Request(operation, current->word, -1, socket);
        if(handle_server_encryption(socket, enc_file)){
            words_processed++;
        }
        else{
            printf("No response from server\n");
        }
        /*struct RPCResponse* response = receive_RPC_response(socket);
        if (response) {
            //handle_server_encryption(response->word, response->key, enc_file);
            handle_server_encryption(enc_file);
            free(response);
            words_processed++;
        }*/
        current = current->next;
    }

    //Single Word
    create_RPC_Request(operation, current->word, -1, socket);
    if(handle_server_encryption(socket, enc_file)){
        words_processed++;
    }
    else{
        printf("No response from server\n");
    }
    /*struct RPCResponse* response = receive_RPC_response(socket);
    if (response) {
        handle_server_encryption(response->word, response->key, enc_file);
        free(response);
        words_processed++;
    }*/

    freeList(head);
    operation = RPC_CLOSE;
    create_RPC_Request(operation, "close", -1, socket);
    struct RPCResponse* close_response = receive_RPC_response(socket);
    if (close_response) {
        free(close_response);
    }
    close(socket);
    printf("Words processed in %s: %d\n", file_name, words_processed);
    pthread_mutex_lock(&total_words_mutex);
    total_words_processed += words_processed;
    pthread_mutex_unlock(&total_words_mutex);
    fclose(enc_file); 
    return 0;
}

void* process_file(void* arg) {
    // Cast the argument to the filename
    const char* file_name = (const char*)arg;
    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    size_t len = strlen(MAGIC_NUMBER);

    char buf[len + 1]; 
    if (fread(buf, 1, len, file) == len) {
        buf[len] = '\0';
        fclose(file);
        if (strcmp(buf, MAGIC_NUMBER) == 0) {
            // First X characters match target string, call decryptFile
            decryptFile(file_name);
        } else {
            encryptFile(file_name);
        }
    }   
    else {
        encryptFile(file_name);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s IP PORT file1 file2 ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* ip = argv[1];
    const char* server_port_str = argv[2];
    const int port = atoi(server_port_str);

    if (port <= 0) {
        fprintf(stderr, "Invalid port number\n");
        return EXIT_FAILURE;
    }

    // Set the global variables
    strncpy(SERVER_IP, ip, sizeof(SERVER_IP) - 1);
    SERVER_IP[sizeof(SERVER_IP) - 1] = '\0';
    SERVER_PORT = port;
    pthread_t threads[argc - 3];

    int i;

    for (i = 3; i < argc; i++){
        if (pthread_create(&threads[i - 3], NULL, process_file, (void*)argv[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for ( i = 0; i < argc - 3; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

    printf("Total words processed by all threads: %d\n", total_words_processed);

    return 0;
}
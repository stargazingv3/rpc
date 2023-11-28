#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

//#define SERVER_IP "10.100.240.204"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAGIC_NUMBER "aa231fd13)@!!@!%asdj3jfddjlmbt"
//const char* targetString = "aa231fd13)@!!@!%asdj3jfddjlmbt";
pthread_mutex_t total_words_mutex = PTHREAD_MUTEX_INITIALIZER;
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
    //char word[];
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
        *head = newNode;
    } else {
        Node* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

// Function to free the list
void freeList(Node* head) {
    Node* tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp->word);
        free(tmp);
    }
}
 
void create_RPC_Request(enum RPCOperation operation, const char* word, int key, int socket){
    struct RPCRequest* request = malloc(sizeof(struct RPCRequest));
    request->operation = operation;
    if(word == NULL){
        //request->word = NULL;
        //request->word = "NULL";
        strncpy(request->word, "NULL", sizeof(request->word) - 1);
        request->word[sizeof(request->word) - 1] = '\0';
        request->key = -1;
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
        // Handle error
    }
    printf("Thread %ld has sent request with word %s to server\n", pthread_self(), request->word);
    free(request);
}

void free_RPC_Request(struct RPCRequest* request) {
    if (request) {
        //free(request->word); // Free the memory for the word field
        free(request); // Free the RPCRequest structure itself
    }
}

int connect_to_server(){
    int client_socket;
	struct sockaddr_in server_addr;
	
	// Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
	else{
		printf("Socket creation successful\n");
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
	else{
		printf("Successful connection to server\n");
	}
    //send(client_socket, &request, sizeof(request), 0);
    return client_socket;
}

void send_RPC_request(int socket, struct RPCRequest* request){
    //send(socket, request, sizeof(*request), 0);
    if (send(socket, request, sizeof(*request), 0) == -1) {
        perror("send failed");
        // Handle error
    }
}

// Function to receive an RPC response (client stub) hey
struct RPCResponse* receive_RPC_response(int socket_fd) {
    //char buffer[1024];
    struct RPCResponse* response = malloc(sizeof(struct RPCResponse));
    ssize_t bytes_received = recv(socket_fd, response, sizeof(struct RPCResponse), 0);
    //while (recv(socket_fd, response, sizeof(struct RPCResponse), 0) > 0) {
    //ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        if(response){
        //buffer[bytes_received] = '\0';
        //return strdup(buffer);
        
            return response;
        } else {
            //printf("Buffer received: %s", strdup(buffer));
            perror("Error receiving response from server");
            return NULL;
        }
    }
    perror("Error receiving response from server");
    free(response);
    return NULL;
}

void* handle_server_decryption(char* word, FILE* file){
    
    if (file == NULL) {
        perror("Invalid file pointer");
        return NULL;
    }

    fprintf(file, "%s\n", word);
    return NULL;
}

void* handle_server_encryption(char* word, int key, FILE* file){
   
    if (file == NULL) {
        fprintf(stderr, "Invalid file pointer\n");
        return NULL;
    }

    fprintf(file, "%s : %d\n", word, key); // Write directly to the passed file
    return NULL;
}

void* decryptFile(const char* file_name){
    char line[256]; // Assuming a maximum line length of 255 characters
    char word[256];
    int number;
    int words_processed = 0;

    printf("Decrypting file %s\n", file_name);
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Failed to open input file");
        return NULL;
    }

    // Ignore the first line
    if (fgets(line, sizeof(line), file) == NULL) {
        //fclose(file);
        //return NULL;
    }

    // Read and process subsequent lines
    enum RPCOperation operation = RPC_DECRYPT;
    //struct RPCRequest* request;

    int socket = connect_to_server();
    printf("\nThread %ld connected to server\n", pthread_self());

    size_t file_size = strlen(file_name) + strlen("_dec") + 1;
    char* name = (char*)malloc(file_size);
    if(name == NULL){
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        return NULL;
    }

    snprintf(name, file_size, "%s_dec", file_name);
    /*FILE* file2 = fopen(name, "w");
    fclose(file2);*/
    FILE* dec_file = fopen(name, "w");
    
    if (dec_file == NULL) {
        perror("Failed to open decryption output file");
        fclose(file);
        return NULL;
    }
    printf("Thread %ld created file %s\n\n", pthread_self(), name);

    while (fgets(line, sizeof(line), file) != NULL) {
        //printf("Thread %ld started reading in from file\n", pthread_self());
        if (sscanf(line, "%s : %d", word, &number) == 2) {
            //printf("Thread %ld has read in %s and %d from file\n", pthread_self(), word, number);
            // Successfully parsed word and number
            //printf("Word: %s, Number: %d\n", word, number);
            //request = create_RPC_Request(operation, word, number);
            //struct RPCRequest* request = create_RPC_Request(operation, word, number, socket);
            create_RPC_Request(operation, word, number, socket);
            //send_RPC_request(request);
            //printf("Sending request for operation: %d, word: %s, with key: %d size: %zu\n", request->operation, request->word, request->key, sizeof(request));
            //send_RPC_request(socket, request);

            //free_RPC_Request(request);
            //char* response = receive_RPC_response(socket);
            struct RPCResponse* response = receive_RPC_response(socket);
            if (response) {
                //printf("Server Response:\n%s\n", response);
                printf("\nThread %ld, Received response %s, trying to write to file %p\n", pthread_self(), response->word, dec_file);
                handle_server_decryption(response->word, dec_file);
                free(response);
                words_processed++;
            }
        } else {
            // Parsing failed
            printf("Error parsing line: %s", line);
        }
    }

    operation = RPC_CLOSE;
    //request = create_RPC_Request(operation, "close", -1);
    //struct RPCRequest* request = create_RPC_Request(operation, word, number, socket);
    //send_RPC_request(socket, request);
    create_RPC_Request(operation, word, number, socket);
    //free_RPC_Request(request);
    // Free the allocated memory for new_file_name
    close(socket);
    free(name);
    //free(name);
    printf("Finished writing to %s_dec\n", file_name);
    printf("Thread %ld: Words processed: %d\n", pthread_self(), words_processed);
    pthread_mutex_lock(&total_words_mutex);
    total_words_processed += words_processed;
    pthread_mutex_unlock(&total_words_mutex);
    printf("Finished Updating global count to %d\n", total_words_processed);
    fclose(dec_file);
    fclose(file);
    //printf("Finished processing %s, exiting\n", file_name);
    return NULL;
}

void* encryptFile(const char* file_name){
    printf("Encrypting file %s\n", file_name);
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
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        //printf("Read-in word: %s\n", buffer);
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
    //FILE* file = fopen(name, "w");
    FILE* enc_file = fopen(name, "w");
    free(name);
    if (enc_file == NULL) {
        fprintf(stderr, "Failed to open encryption output file\n");
        return NULL;
    }

    size_t entry_size = strlen(MAGIC_NUMBER); 

    char entry[256];
    snprintf(entry, sizeof(entry), "%s\n", MAGIC_NUMBER);

    // Write the formatted data to the file
    fprintf(enc_file, "%s", entry);
    printf("Writing %s to file\n", entry);

    // Close the file
    //fclose(enc_file);

    // Free the allocated memory for new_file_name
    //free(name);

    int socket = connect_to_server();

    Node* current = head;
    current = head;
    while(current->next){
        enum RPCOperation operation = RPC_ENCRYPT;
	    //operation = RPC_ENCRYPT;
        //printf("Creating request with word: %s\n", current->word);
        printf("Doing operation: %d\n", operation);
        create_RPC_Request(operation, current->word, -1, socket);
        //struct RPCRequest* request = create_RPC_Request(operation, current->word, -1);
        //printf("Sending Request: %s\n", request->word);
        //int socket = send_RPC_request(request);
      //  printf("Sending request for operation: %d, word: %s, size: %zu\n", request->operation, request->word, sizeof(request));
        //send_RPC_request(socket, request);
        //char* response = receive_RPC_response(socket);
        struct RPCResponse* response = receive_RPC_response(socket);
        if (response) {
            //printf("Server Response:\n%s\n", response);
            handle_server_encryption(response->word, response->key, enc_file);
            free(response);
            words_processed++;
        }
        /*else {
            fprintf(stderr, "\n");
        }*/
        current = current->next;
    }

    //Single Word
    enum RPCOperation operation = RPC_ENCRYPT;
	    //operation = RPC_ENCRYPT;
        //printf("Creating request with word: %s\n", current->word);
        //printf("Doing operation: %d\n", operation);
        //struct RPCRequest* request = create_RPC_Request(operation, current->word, -1);
        //printf("Sending Request: %s\n", request->word);
        //int socket = send_RPC_request(request);
       // printf("Sending request for operation: %d, word: %s, size: %zu\n", request->operation, request->word, sizeof(request));
        //send_RPC_request(socket, request);
    create_RPC_Request(operation, current->word, -1, socket);
    struct RPCResponse* response = receive_RPC_response(socket);
    if (response) {
        //printf("Server Response:\n%s\n", response);
        handle_server_encryption(response->word, response->key, enc_file);
        free(response);
        words_processed++;
    }

    freeList(head);
    printf("Finished writing to %s_enc\n", file_name);

    operation = RPC_CLOSE;
    //request = create_RPC_Request(operation, "close", -1);
    //send_RPC_request(socket, request);
    create_RPC_Request(operation, "close", -1, socket);
    struct RPCResponse* close_response = receive_RPC_response(socket);
    if (close_response) {
        free(close_response);
    }
    close(socket);

    //close(socket);
    printf("Thread %ld: Words processed: %d\n", pthread_self(), words_processed);
    pthread_mutex_lock(&total_words_mutex);
    total_words_processed += words_processed;
    pthread_mutex_unlock(&total_words_mutex);
    fclose(enc_file); 
    return 0;
}

void* process_file(void* arg) {
	printf("Thread %ld, Starting to process file %s\n", pthread_self(), (const char*)arg);
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
            printf("Finished decrypting %s\n", file_name);
        } else {
            // First X characters do not match, call encryptFile
            encryptFile(file_name);
            printf("Finished encrypting %s\n", file_name);
        }
    }   
    else {
        encryptFile(file_name);
        //printf("Read in: %s\n", buf);
        //fprintf(stderr, "Error reading from the file.\n");
    }

    printf("Finished processing %s, exiting\n", file_name);
    return NULL;
}

int isEqualIgnoreCase(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        if (tolower(*str1) != tolower(*str2)) {
            return 0; // Not equal
        }
        str1++;
        str2++;
    }
    
    // Check if both strings have the same length
    return (*str1 == '\0' && *str2 == '\0');
}

int main(int argc, char* argv[]) {
    // ...
	printf("Arguments: %d\n", argc);
    int i;
	for (i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    pthread_t threads[argc - 1];

    for (i = 1; i < argc; i++){
        printf("\nStarting to process %s\n", argv[i]);
        //process_file((void*) argv[i]);
        if (pthread_create(&threads[i - 1], NULL, process_file, (void*)argv[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for ( i = 0; i < argc - 1; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

    printf("Total words processed by all threads: %d\n", total_words_processed);

    return 0;
}
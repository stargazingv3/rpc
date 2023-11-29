#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <wchar.h>
#include <regex.h>

char SERVER_IP[16];  // Assuming IPv4 address
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
    newNode->word = strdup(word); // Duplicate the word
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

void freeLineData(struct LineData* data) {
    free(data->content);
    free(data);
}

//Client Stub
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
        // Handle error
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

//Client Stub
// Function to receive an RPC response. Fills out a RPCResponse struct to send back after
// waiting for server response on specified socket
struct RPCResponse* receive_RPC_response(int socket_fd) {
    struct RPCResponse* response = malloc(sizeof(struct RPCResponse));
    ssize_t bytes_received = recv(socket_fd, response, sizeof(struct RPCResponse), 0);
    if (bytes_received > 0) {
        if(response){
            return response;
        } else {
            perror("Error receiving response from server");
            return NULL;
        }
    }
    free(response);
    return NULL;
}

//Client Stub
//Processes the output of the server's decryption by saving it to a file
void* handle_server_decryption(char* word, FILE* file){
    
    if (file == NULL) {
        perror("Invalid file pointer");
        return NULL;
    }

    fprintf(file, "%s ", word);
    return NULL;
}

//Client Stub
//Processes the output of the server's encryption by saving it to a file
//Replaces new line characters in encrypted word to keep the output on the same line
void* handle_server_encryption(char* word, int key, FILE* file) {
    if (file == NULL) {
        fprintf(stderr, "Invalid file pointer\n");
        return NULL;
    }

    // Encodes newline characters before writing to the file
    // Necessary as otherwise output will contain multiple lines and also makes it
    // Harder to parse input.
    size_t word_length = strlen(word);
    size_t i;
    fprintf(file, "{");
    for (i = 0; i < word_length; i++) {
        if (word[i] == '\n') {
            // If the key contains a new line, encode as, "\n" and write to file
            fprintf(file, "\\n");
        } else { 
            //Write character to file
            fputc(word[i], file);
        }
    }

    fprintf(file, " : %d},", key); // Write the key after the encoded word
    return NULL;
}

//Function used to parse tokens containing the encrypted word and key
//The given token will follow the structure {encrypted : key}. Once parsed, it will
//Return a LineData struct containing the word and key. If the inputted token doesn't
//Follow the format required, it will not process
struct LineData* parseLine(const char* line) {
    struct LineData* data = (struct LineData*)malloc(sizeof(struct LineData));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    // Allows the encrypted word to be any length
    data->content = strdup(line);
    if (data->content == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        free(data);
        return NULL;
    }

    // Exclude the first and last characters from content as they are {}
    size_t lineLength = strlen(line);
    if (lineLength >= 2) {
        data->content = (char*)malloc(lineLength - 1);
        if (data->content == NULL) {
            fprintf(stderr, "Memory allocation error\n");
            freeLineData(data);
            return NULL;
        }
        //
        strncpy(data->content, line + 1, lineLength - 2);
        data->content[lineLength - 2] = '\0';  // Null-terminate
    } else {
        fprintf(stderr, "Invalid format1: %s\n", line);
        freeLineData(data);
        return NULL;
    }

    // Parse the key from the input
    char* keyStr = strstr(data->content, " : ");
    if (keyStr == NULL) {
        fprintf(stderr, "Invalid format2: %s\n", line);
        freeLineData(data);
        return NULL;
    }

    *keyStr = '\0';
    keyStr += 3;     // Move past " : "
    if (sscanf(keyStr, "%d", &(data->key)) != 1) { //Writes extracted key to struct
        fprintf(stderr, "Error parsing key: %s\n", line);
        freeLineData(data);
        return NULL;
    }

    return data;
}

//Client stub to handle all the operations needed to decrypt a file
//Responsible for parsing tokens from the file, creating appropriate RPCRequests, 
//Sending a request to close connection when done
//Is the start and end of all the processing which needs to be done to decrypt a file
void* decryptFile(const char* file_name) {
    char line[256];
    int words_processed = 0;

    //Required to open as a binary file as file will contain non-utf8 codes
    FILE* file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("Failed to open input file");
        return NULL;
    }

    // Ignore the first line as it is just the magic number
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return NULL;
    }

    // Read and process subsequent lines
    enum RPCOperation operation = RPC_DECRYPT;
    int socket = connect_to_server();

    //Creates output file
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

    //Parses through the input file, searching for tokens as dictated by {}. When found,
    //Sends token to parseLine to extract word, key from token and then create and handle
    //Decryption request to and from server
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '{') { //Start of token
            // Read characters until '}' for end of token
            size_t buffer_size = 256;
            char* buffer = (char*)malloc(buffer_size);
            size_t i = 0;

            //Put the entire token in a buffer to be sent for parsing
            buffer[i++] = '{'; 
            while ((c = fgetc(file)) != EOF && c != '}') {
                if (i == buffer_size - 1) {
                    // Resize buffer if needed
                    buffer_size *= 2;
                    buffer = (char*)realloc(buffer, buffer_size);
                }
                buffer[i++] = c;
            }

            if (c == '}') {
                buffer[i++] = '}';  
            }
            buffer[i] = '\0';

            //Create RPCRequest, handle server response, handle parsing failure
            struct LineData* data = parseLine(buffer);
            if (data != NULL) {
                create_RPC_Request(operation, data->content, data->key, socket);
                struct RPCResponse* response = receive_RPC_response(socket);
                if (response) {
                    handle_server_decryption(response->word, dec_file);
                    free(response);
                    words_processed++;
                }
                freeLineData(data);
            } else {
                // Parsing failed for an entry
                printf("Error parsing entry: %s\n", buffer);
                fclose(dec_file);
                fclose(file);
                free(name);
                free(buffer);
                return NULL;
            }

            free(buffer);
        }
    }

    //File has finished processing, send server a close request and clean-up
    operation = RPC_CLOSE;
    create_RPC_Request(operation, line, 0, socket);  // Adjust accordingly if the line needs to be processed
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

//Client stub to handle all the operations needed to encrypt a file
//Responsible for parsing words to encrypt, creating appropriate RPCRequests, 
//Sending a request to close connection when done
//Is the start and end of all the processing which needs to be done to encrypt a file
void* encryptFile(const char* file_name){
    char command[256]; //Used to call word-count to extract words
    FILE *fp;
    char buffer[1024]; // Assumes max word length of 1024
    int words_processed = 0;

    snprintf(command, sizeof(command), "./word-count %s", file_name);

    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Failed to run command");
        exit(1);
    }

    Node* head = NULL; //Used to start a linked list to store all words to process

    while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        appendNode(&head, buffer);
    }

    pclose(fp);

    //Creates file to store output to
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

    //First writes the magic number to the start of the file to dictate that the file 
    //Contains encrypted words.
    char entry[256];
    snprintf(entry, sizeof(entry), "%s\n", MAGIC_NUMBER);
    fprintf(enc_file, "%s", entry);

    int socket = connect_to_server();

    //Start processing words by going through the list and requesting and handling output
    //of encryption from server
    Node* current = head;
    current = head;
    enum RPCOperation operation = RPC_ENCRYPT;
    while(current->next){
        create_RPC_Request(operation, current->word, -1, socket);
        struct RPCResponse* response = receive_RPC_response(socket);
        if (response) {
            handle_server_encryption(response->word, response->key, enc_file);
            free(response);
            words_processed++;
        }
        current = current->next;
    }

    //Necessary to handle case of files with a single word as well as the last word
    //Does the same as above without going through a list
    create_RPC_Request(operation, current->word, -1, socket);
    struct RPCResponse* response = receive_RPC_response(socket);
    if (response) {
        handle_server_encryption(response->word, response->key, enc_file);
        free(response);
        words_processed++;
    }

    //Finished encrypting a file, can send the server a request to close connection and 
    //Cleanup
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

//Driver function which calls appropriate client stubs for given input files
void* process_file(void* arg) {
    const char* file_name = (const char*)arg;
    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Failed to open file");
        printf("Failed for %s\n", file_name);
        return NULL;
    }

    //Use a magic number to determine if a file is encrypted or decrypted.
    //If the first line of a file matches the magic number, it's encrypted and so call
    //The decryption stub, else call the encryption stub
    size_t len = strlen(MAGIC_NUMBER);
    char buf[len + 1]; 
    if (fread(buf, 1, len, file) == len) { // First line matches the length of the magic number
        buf[len] = '\0';
        fclose(file);
        if (strcmp(buf, MAGIC_NUMBER) == 0) {
            decryptFile(file_name);
        } else { //Coincidence, not actually the magic number
            encryptFile(file_name);
        }
    }   
    else {
        encryptFile(file_name);
    }

    return NULL;
}

//Main function. Ensures proper arguments are given, creates threads for each file
int main(int argc, char* argv[]) {
    if (argc < 5) {
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

    //Wait for all threads to finish before printing out the total processed words
    for ( i = 0; i < argc - 3; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

    printf("Total words processed by all threads: %d\n", total_words_processed);

    return 0;
}
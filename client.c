#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define SERVER_IP "10.100.240.204"
#define SERVER_PORT 8080
#define MAGIC_NUMBER "aa231fd13)@!!@!%asdj3jfddjlmbt"
//const char* targetString = "aa231fd13)@!!@!%asdj3jfddjlmbt";


// Define the RPC protocol operations
enum RPCOperation {
    RPC_ENCRYPT,
    RPC_DECRYPT,
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

struct RPCRequest* create_RPC_Request(enum RPCOperation operation, const char* word, int key){
    struct RPCRequest* request = malloc(sizeof(struct RPCRequest));
    request->operation = operation;
    //request->word = strcpy(requword);
    //strcpy(request->word, word);
    //printf("Recieved: %s, Setting word to: %s\n", text, strdup(text));
    //printf("%s\n", request.word);
    strncpy(request->word, word, sizeof(request->word) - 1);
    request->word[sizeof(request->word) - 1] = '\0';
    if(key > 0){
        request->key = key;
    }
    return request;
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
    send(socket, request, sizeof(*request), 0);
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
    return NULL;
}

void* handle_server_decryption(char* word, char* file_name){
    size_t file_size = strlen(file_name) + strlen("_dec") + 1;
    char* name = (char*)malloc(file_size);
    if(name == NULL){
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

    snprintf(name, file_size, "%s_dec", file_name);
    FILE* file = fopen(name, "a");

    size_t entry_size = strlen(word); 

    char entry[256];
    snprintf(entry, sizeof(entry), "%s\n", word);

    // Write the formatted data to the file
    fprintf(file, "%s", entry);
    printf("Writing %s to file\n", entry);

    // Close the file
    fclose(file);

    // Free the allocated memory for new_file_name
    free(name);
    return;
}

void* handle_server_encryption(char* word, int key, char* file_name){
    size_t file_size = strlen(file_name) + strlen("_enc") + 1;
    char* name = (char*)malloc(file_size);
    if(name == NULL){
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

    snprintf(name, file_size, "%s_enc", file_name);
    FILE* file = fopen(name, "a");

    size_t entry_size = strlen(word); 

    char entry[256];
    snprintf(entry, sizeof(entry), "%s : %d\n", word, key);

    // Write the formatted data to the file
    fprintf(file, "%s", entry);
    printf("Writing %s to file\n", entry);

    // Close the file
    fclose(file);

    // Free the allocated memory for new_file_name
    free(name);
    return;
}

void* decryptFile(char* file_name){
    printf("Decrypting file %s\n", file_name);
    FILE *file = fopen(file_name, "r");

    char line[256]; // Assuming a maximum line length of 255 characters
    char word[256];
    int number;

    // Ignore the first line
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return 1;
    }

    // Read and process subsequent lines
    enum RPCOperation operation = RPC_DECRYPT;
    struct RPCRequest* request;

    int socket = connect_to_server();

    size_t file_size = strlen(file_name) + strlen("_dec") + 1;
    char* name = (char*)malloc(file_size);
    if(name == NULL){
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

    snprintf(name, file_size, "%s_dec", file_name);
    FILE* file2 = fopen(name, "w");
    fclose(file2);

    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%s : %d", word, &number) == 2) {
            // Successfully parsed word and number
            printf("Word: %s, Number: %d\n", word, number);\
            request = create_RPC_Request(operation, word, number);
            //send_RPC_request(request);
            printf("Sending request for operation: %d, word: %s, with key: %d size: %d\n", request->operation, request->word, request->key, sizeof(request));
            send_RPC_request(socket, request);
            //char* response = receive_RPC_response(socket);
            struct RPCResponse* response = receive_RPC_response(socket);
            if (response) {
                //printf("Server Response:\n%s\n", response);
                handle_server_decryption(response->word, file_name);
                free(response);
            }
        } else {
            // Parsing failed
            printf("Error parsing line: %s", line);
        }
    }

    // Free the allocated memory for new_file_name
    free(name);

}

void* encryptFile(char* file_name){
    printf("Encrypting file %s\n", file_name);
    char command[256];
    FILE *fp;
    char buffer[1024]; // Adjust buffer size as needed

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
        appendNode(&head, buffer);
    }

    pclose(fp);

    size_t file_size = strlen(file_name) + strlen("_enc") + 1;
    char* name = (char*)malloc(file_size);
    if(name == NULL){
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

    snprintf(name, file_size, "%s_enc", file_name);
    FILE* file = fopen(name, "w");

    size_t entry_size = strlen(MAGIC_NUMBER); 

    char entry[256];
    snprintf(entry, sizeof(entry), "%s\n", MAGIC_NUMBER);

    // Write the formatted data to the file
    fprintf(file, "%s", entry);
    printf("Writing %s to file\n", entry);

    // Close the file
    fclose(file);

    // Free the allocated memory for new_file_name
    free(name);

    int socket = connect_to_server();

    Node* current = head;
    current = head;
    while(current->next){
        enum RPCOperation operation = RPC_ENCRYPT;
	    //operation = RPC_ENCRYPT;
        //printf("Creating request with word: %s\n", current->word);
        printf("Doing operation: %d\n", operation);
        struct RPCRequest* request = create_RPC_Request(operation, current->word, -1);
        //printf("Sending Request: %s\n", request->word);
        //int socket = send_RPC_request(request);
        printf("Sending request for operation: %d, word: %s, size: %d\n", request->operation, request->word, sizeof(request));
        send_RPC_request(socket, request);
        //char* response = receive_RPC_response(socket);
        struct RPCResponse* response = receive_RPC_response(socket);
        if (response) {
            //printf("Server Response:\n%s\n", response);
            handle_server_encryption(response->word, response->key, file_name);
            free(response);
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
        printf("Doing operation: %d\n", operation);
        struct RPCRequest* request = create_RPC_Request(operation, current->word, -1);
        //printf("Sending Request: %s\n", request->word);
        //int socket = send_RPC_request(request);
        printf("Sending request for operation: %d, word: %s, size: %d\n", request->operation, request->word, sizeof(request));
        send_RPC_request(socket, request);
        struct RPCResponse* response = receive_RPC_response(socket);
        if (response) {
            //printf("Server Response:\n%s\n", response);
            handle_server_encryption(response->word, response->key, file_name);
            free(response);
        }

    freeList(head);
    printf("Finished writing to %s_enc\n", file_name);
    return 0;
}

void* process_file(void* arg) {
	printf("Starting to process file %s\n", arg);
    // Cast the argument to the filename
    const char* file_name = (const char*)arg;
    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    size_t len = strlen(MAGIC_NUMBER);

    char buf[len + 1]; 
    if (fread(buf, 1, len, file) == len) {
        buf[len] = '\0';
        if (strcmp(buf, MAGIC_NUMBER) == 0) {
            // First X characters match target string, call decryptFile
            decryptFile(file_name);
        } else {
            // First X characters do not match, call encryptFile
            encryptFile(file_name);
        }
    }   
    else {
        encryptFile(file_name);
        //printf("Read in: %s\n", buf);
        //fprintf(stderr, "Error reading from the file.\n");
    }

    fclose(file);
    return 0;

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
	for (int i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    // Iterate through the input files
    /*for (int i = 1; i < argc; i += 2) {
        // ...

        const char* file_name = argv[i+1];
		const char* operation = argv[i];
		const char* key_file;
		
		pthread_t thread;*/
		
		/*if(isEqualIgnoreCase(operation, "decrypt")){
			key_file = argv[i+2];
			i += 1;
			if (pthread_create(&thread, NULL, process_file, (void*)file_name) != 0) {
				perror("Error creating thread");
				continue; // Continue to the next file
			}
		}
		else{
			printf("Creating thread to encrypt file %s\n", file_name);
			if (pthread_create(&thread, NULL, process_file, (void*)argv[1]) != 0) {
				perror("Error creating thread");
				continue; // Continue to the next file
			}
			else{
				printf("Thread created successfully for file: %s and operation: %s \n", file_name, operation);
			}*/
        process_file((void*) argv[1]);

        // Create a thread for each file
        //pthread_t thread;
        

        // You can optionally join the threads here if you want to wait for them to finish before proceeding.
		//}
	//}

    // Cleanup and exit
    //close(client_socket);
    return 0;
}

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


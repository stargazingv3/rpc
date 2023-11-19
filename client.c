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
    char text[1024];
    int key;
};

typedef struct Node {
    char* word;
    struct Node* next;
} Node;

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

void* decryptFile(char* file_name){
    printf("Decrypting file %s\n", file_name);
}

void* encryptFile(char* file_name){
    printf("Encrypting file %s\n", file_name);
    char command[256];
    FILE *fp;
    char buffer[1024]; // Adjust buffer size as needed

    snprintf(command, sizeof(command), "./word-count %s", file_name);

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

    // Print and free the list
    Node* current = head;
    while (current != NULL) {
        printf("Input: %s\n", current->word);
        current = current->next;
    }

    freeList(head);
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
        fprintf(stderr, "Error reading from the file.\n");
    }

    fclose(file);
    //return 0;

	/*enum RPCOperation operation;
	
	operation = RPC_ENCRYPT;
    // Create a client socket and connect to the server (similar to the main function)
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
	
        // Read text from the specified file
        FILE* file = fopen(file_name, "r");
        if (file == NULL) {
            perror("Error opening file");
            return;
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
    //}

    // Cleanup and exit
    close(client_socket);*/
    return 0;

    //return NULL;
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


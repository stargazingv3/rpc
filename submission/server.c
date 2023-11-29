#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h>

#define MAX_SHIFT 9 //Assignment-specified limit of shifting up til 9
//A 2048-bit RC4 Key.
#define RC4Key "0DDCD6C5BEA76DDB9BD70EB3EFC053B61000ABE01EC4BE2FD76DA841D549E7E225BDA7E46414BF00EBCDB3DA8D07909E073B7E26FF3D55D6AAFD177F47FE616CBB0850201C102007DDD3E16BDA7109E2AD8708ACC45D836F5B9AEEA299500E54585F74756F947C4C685EB742CFC0247C482C290C8AAC7BE5466A87DFBA953412F4A887633D04B0A56267E731280CAE7038D77CC283F8A7C9622EA91CC4DD2EB885B61CC2BACC671C334F4D5B5BFBCB93D248565540FD1FA22CC8BEF0A5ECA82AA2C4ED5C905478C4A3C61FFEC1EB929433E8E973E5081511D0D30175BFAAA0626E8DBEFFE137C385FDE283BECD155200FD3C73E34488F4155BF68A1BA02A7D0E"

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
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

struct Encrypted {
    char* word;
    int key;
};

//Server Stub
//Creates and sends an RPC response containing the encrypted/decrypted results
//Sends the same response format for both encryption and decryption, as the client
//Handles whether the key is necessary or not
struct RPCResponse* create_RPC_Response(struct Encrypted* result, int socket) {
    struct RPCResponse* response = malloc(sizeof(struct RPCResponse));
    if (response == NULL) {
        perror("Memory allocation failed");
        // Handle error
        exit(EXIT_FAILURE);
    }

    size_t copy_length = sizeof(response->word) - 1;
    strncpy(response->word, result->word, copy_length);
    response->word[copy_length] = '\0';

    response->key = result->key;

    if (send(socket, response, sizeof(*response), 0) == -1) {
        perror("send failed");
        // Handle error
    }

    return response;
}


// Function to generate a random number between 1 and the specified max shift
int generate_random_key() {
    return (rand() % MAX_SHIFT) + 1;
}

void swap(unsigned char *a, unsigned char *b) {
    unsigned char temp = *a;
    *a = *b;
    *b = temp;
}

// Function to initialize the RC4 S-box
// The substitution box is a sort of look-up table which is used when generating the keystream
// During other parts of the RC4 encryption steps. It contains 256 values where the order is
// Moved around based on the key length, and is used to generate the keystream.
//The S-box is the overall first step for RC4
void initialize_sbox(unsigned char *sbox, const unsigned char *key, size_t key_length) {
    int i;

    // Step 1: Initialize the S-box with values 0 to 255
    for (i = 0; i < 256; i++) {
        sbox[i] = i;
    }

    // Step 2: Randomize the S-box based on the provided key length
    size_t j = 0;
    for (i = 0; i < 256; i++) {
        j = (j + sbox[i] + key[i % key_length]) % 256;
        swap(&sbox[i], &sbox[j]);
    }
}

// Function to perform RC4 encryption or decryption
void rc4_crypt(const unsigned char *input, size_t length, unsigned char *output, const unsigned char *key, size_t key_length) {
    unsigned char sbox[256];

    // Step 1: Initialize the S-box based on the provided key
    initialize_sbox(sbox, key, key_length);

    size_t i = 0, j = 0, k;

    // Step 2: Generate a keystream based on the sbox and XOR it with the input data
    for (k = 0; k < length; k++) {
        i = (i + 1) % 256;
        j = (j + sbox[i]) % 256;
        //i and j are used as indexes to go through the sbox, where their role is to 
        //further randomize the sbox based on the key itself, instead of just the length
        swap(&sbox[i], &sbox[j]);

        // Generates the next byte of the keystream based on the updated sbox
        unsigned char keystream = sbox[(sbox[i] + sbox[j]) % 256];
        // XOR the input byte with the keystream byte to produce the output byte
        output[k] = input[k] ^ keystream;
    }
}

// Function to perform RC4 encryption
// Given the word and key to use, identifies necessary parameters and passes it on to
// rc4_crypt which does the actual encryption
char *rc4_encrypt(const char *text, const char *key) {
    size_t text_length = strlen(text);
    unsigned char *encrypted = (unsigned char *)malloc(text_length + 1);
    if (encrypted == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    rc4_crypt((const unsigned char *)text, text_length, encrypted, (const unsigned char *)key, strlen(key));
    encrypted[text_length] = '\0';

    return (char *)encrypted;
}

// Function to apply RC4 decryption
// Since it is symetric encryption, same steps as encryption. Hardcoded to use #define key
char* rc4_decrypt(const char* text, const char* key) {
    return rc4_encrypt(text, RC4Key);
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

        if (isalpha(c)) { // only alphabetic characters get shifted
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

// Encrypts a given client word
// First caesar shifts the word, then applies RC4 encryption to it and returns result
struct Encrypted* server_encrypt(const char* text) {
    int key = generate_random_key();
    char* rc4_encrypted;

    if (text == NULL) {
        perror("Input text is NULL");
        return NULL;
    }

    char* encrypted_text = caesar_cipher_encrypt(text, key);

    rc4_encrypted = rc4_encrypt(encrypted_text, RC4Key);

    //Stores the result and caesar key in a struct to return with.
    struct Encrypted* ret = (struct Encrypted*)malloc(sizeof(struct Encrypted));

    char buffer[1024]; //Stores the resulting encrypted word
    snprintf(buffer, sizeof(buffer), "%s", rc4_encrypted);
    ret->word = strdup(buffer);

    // Check for memory allocation failure
    if (ret->word == NULL) {
        perror("Memory allocation failed");
        free(ret); // Free the allocated struct if strdup fails
        return NULL;
    }
    ret->key = key;

    return ret;
}

//This function is needed to replace the newline encoding with actual new lines
//Before passing it to decryp
char* undo_server_encryption(const char* encoded_text) {
    if (encoded_text == NULL) {
        perror("Input text is NULL");
        return NULL;
    }

    size_t encoded_length = strlen(encoded_text);
    char* original_text = (char*)malloc(encoded_length + 1);  // +1 for the null terminator

    if (original_text == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    // Decode the text
    size_t i, j;
    for (i = 0, j = 0; i < encoded_length; i++) {
        if (encoded_text[i] == '\\' && i + 1 < encoded_length) {
            // Check for the encoded newline character
            if (encoded_text[i + 1] == 'n') {
                original_text[j++] = '\n';
                i++;  // Skip the 'n' character
            } else {
                // Copy other escaped characters as-is
                original_text[j++] = encoded_text[i];
            }
        } else {
            // Copy non-escaped characters as-is
            original_text[j++] = encoded_text[i];
        }
    }

    original_text[j] = '\0';

    return original_text;
}


// Decrypts a given client word
// First decrypts using RC4 then caesar to undo order of encryption
struct Encrypted* server_decrypt(const char* text, int key) {
    if (text == NULL) {
        perror("Input text is NULL");
        return NULL;
    }
    char* text2 = undo_server_encryption(text);
    char* rc4_decrypted = rc4_decrypt(text2, "RC4_KEY");
    char* decrypted_text = caesar_cipher_decrypt(rc4_decrypted, key);

    //Stores the result in a struct to return with.
    struct Encrypted* ret = (struct Encrypted*)malloc(sizeof(struct Encrypted));
    ret->word = decrypted_text;
    ret->key = key;

    return ret;
}
	
//Server stub to handle and process client requests
//Checks the operation type to determine if client wants to encrypt/decrypt a word
//Or close the connection and then calls the appropriate helper functions
struct RPCResponse* handle_rpc_request(const struct RPCRequest* request, int socket) {
    struct Encrypted* result = malloc(sizeof(struct Encrypted));
    struct RPCResponse* response;
    switch (request->operation) {
        case RPC_ENCRYPT:
            result = server_encrypt(request->word);
            response = create_RPC_Response(result, socket);
            return response;    
        case RPC_DECRYPT:
            result = server_decrypt(request->word, request->key);
            response = create_RPC_Response(result, socket);
            return response;
        case RPC_CLOSE:
            response = malloc(sizeof(struct RPCResponse));
            strcpy(response->word, "NULL");
            response->key = -1;
            return response;
        default:
			printf("Given empty RPCRequest\n");
            break;
    }
    return NULL;
}

// Server stub to handle a client connection
// Responsible for processing a client connection as long as receiving requests from it
// Upon receiving a request, fills out a RPCRequest struct to pass it to a stub to process
// The request accordingly. Closes connection upon receiving a client request to do so or
// Upon timeout due to no responses
void* handle_client(void* client_socket) {
	
    int socket_fd = *(int*)client_socket;
    struct timeval tv;
    int client_words_processed = 0;
    tv.tv_sec = 1; // Can modify how long to wait for a response before closing a connection
    tv.tv_usec = 0;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        perror("setsockopt failed");
        close(socket_fd);
        return NULL;
    }
    struct RPCRequest request;
    struct RPCResponse* response;
    ssize_t bytes_received;
    while ((bytes_received = recv(socket_fd, &request, sizeof(request), 0)) > 0) {
        response = handle_rpc_request(&request, socket_fd);
        if (response) {
            if(response->key == -1){
                printf("\n\nReceived request to close connection\n");
                pthread_mutex_lock(&count_mutex);
                total_words_processed += client_words_processed;
                printf("Total words processed: %d\n", total_words_processed);
                pthread_mutex_unlock(&count_mutex);
                close(socket_fd);
                return NULL;
            }
            client_words_processed++;
        }
        else{
            printf("No response received\n");
        }
    }
    if (bytes_received < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("Lost packet from one of the threads, please quit and re-run the client\n\n");
        } else {
            perror("recv error");
        }
    }   

    //Finished processing a client connection, can cleanup
    pthread_mutex_lock(&count_mutex);
    total_words_processed += client_words_processed;
    pthread_mutex_unlock(&count_mutex);
    close(socket_fd);
    return NULL;
}

// Main function
// Responsible for ensuring proper arguments to start, creates a thread for each connection
int main(int argc, char *argv[]) {

    // Check if a port is provided as a command-line argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_socket, new_socket, port;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    int clients_count = 0;
    port = atoi(argv[1]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }
    srand(time(NULL)); // Seeds the RNG for caesare cipher key generation

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == 0) {
        printf("Server is listening on port %d...\n", port);
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

        // Create a thread to handle the client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void*)&new_socket) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }

        clients_count++;
    }

    return 0;
}
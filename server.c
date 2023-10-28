#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void *handle_client(void *client_socket_ptr);

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);  // Use your desired port

    bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    while(1) {
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len);
        
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, &client_socket);
    }

    close(server_socket);
    return 0;
}

void *handle_client(void *client_socket_ptr) {
    int client_socket = *(int *) client_socket_ptr;
    char buffer[1024];
    char handshake[1024];

    // Receive the handshake
    recv(client_socket, handshake, sizeof(handshake)-1, 0);
    printf("Received handshake: %s\n", handshake);  // Print the received handshake message

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        recv(client_socket, buffer, sizeof(buffer)-1, 0);
        if(strcmp(buffer, "exit") == 0) {
            break;
        }

        printf("Received word: %s\n", buffer);  // Print the received word before processing

        // Encrypt or decrypt based on handshake
        for(int i = 0; i < strlen(buffer); i++) {
            if(strcmp(handshake, "encryption") == 0) {
                buffer[i]++;
            } else if(strcmp(handshake, "decryption") == 0) {
                buffer[i]--;
            }
        }

        printf("Processed word: %s\n", buffer);  // Print the processed word

        send(client_socket, buffer, strlen(buffer), 0);
    }

    close(client_socket);
    return NULL;
}


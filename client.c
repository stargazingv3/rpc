#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];
    
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("10.100.240.204");  // Replace with the server IP
    server_addr.sin_port = htons(8080);  // Use the same port as in server

    connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));

    // Send handshake (encryption or decryption)
    printf("Choose service (encryption/decryption): ");
    scanf("%s", buffer);
    send(client_socket, buffer, strlen(buffer), 0);

    while(1) {
        printf("Enter a word (or 'exit' to quit): ");
        scanf("%s", buffer);
        send(client_socket, buffer, strlen(buffer), 0);
        if(strcmp(buffer, "exit") == 0) {
            break;
        }
        recv(client_socket, buffer, sizeof(buffer)-1, 0);
        printf("Received: %s\n", buffer);
    }

    close(client_socket);
    return 0;
}

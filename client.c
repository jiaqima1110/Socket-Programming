#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int socket_desc;
struct sockaddr_in serverM_addr;
char serverM_message[2000], client_message[2000];

void clean_message_buffers();
void setup_TCP();
void find_intervals();
void update();

int main() {
    // Clean message buffers
    clean_message_buffers();

    // Setup TCP socket
    setup_TCP();

    // Receive and forward msg
    while (1) {
        find_intervals();
        update();
        clean_message_buffers();
        printf("-----Start a new request-----\n");
    }

    // Close the socket
    close(socket_desc);

    return 0;
}

void clean_message_buffers() {
    memset(serverM_message, '\0', sizeof(serverM_message));
    memset(client_message, '\0', sizeof(client_message));
}

void setup_TCP() {
    // Set port and IP for the serverM
    serverM_addr.sin_family = AF_INET;
    serverM_addr.sin_port = htons(24941);
    serverM_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    // Send connection request to server
    if (connect(socket_desc, (struct sockaddr*)&serverM_addr, sizeof(serverM_addr)) < 0) {
        printf("Unable to connect\n");
    }

    printf("Client is up and running.\n");
}

void find_intervals() {
    // Get input names from user
    printf("Please enter the usernames to check schedule availability:\n");
    fgets(client_message, sizeof(client_message), stdin);

    // Send names to serverM
    if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
        printf("Unable to send message to serverM\n");
    } else {
        printf("Client finished sending the usernames to Main Server.\n");
    }

    // If some usernames do not exit: receive the serverM's response
    if (recv(socket_desc, serverM_message, sizeof(serverM_message), 0) < 0) {
        printf("Error while receiving server's msg\n");
    } else if (strcmp(serverM_message, "all exist") != 0) {
        printf("Client received the reply from Main Server using TCP over port %i: %s\n",
               ntohs(serverM_addr.sin_port), serverM_message);
    }

    // Receive the final result
    if (recv(socket_desc, serverM_message, sizeof(serverM_message), 0) < 0) {
        printf("Error while receiving server's msg\n");
    } else if (strcmp(serverM_message, "No existed clients.") == 0) {
        printf("Client received the reply from Main Server using TCP over port %i: %s\n",
               ntohs(serverM_addr.sin_port), serverM_message);
    } else {
        printf("Client received the reply from Main Server using TCP over port %i:\nTime intervals %s.\n",
               ntohs(serverM_addr.sin_port), serverM_message);
    }
}

void update() {

}

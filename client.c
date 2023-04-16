#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int socket_desc;
struct sockaddr_in serverM_addr, client_addr;
socklen_t len = sizeof(client_addr);
char serverM_message[4000],
     client_message[4000],
     input[4000],
     names[4000];

int slots[51][2];

void clean_message_buffers();
void setup_TCP();
int find_intervals();
void update();

void decode_info(char message[4000]);
void init_slots();
int check_input(char input[4000]);

int main() {
    // Clean message buffers
    clean_message_buffers();

    // Setup TCP socket
    setup_TCP();

    // Receive and forward msg
    while (1) {
        int result = find_intervals();
        if (result != -1) {
            update();
        }
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
    memset(input, '\0', sizeof(input));
    memset(names, '\0', sizeof(names));
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

    // Retrieve client's port
    if (getsockname(socket_desc, (struct sockaddr*)&client_addr, &len) == -1) {
        perror("Error getting client's socket port");
    }

    printf("Client is up and running.\n");
}

int find_intervals() {
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
               ntohs(client_addr.sin_port), serverM_message);
    }

    // Receive the final result
    if (recv(socket_desc, serverM_message, sizeof(serverM_message), 0) < 0) {
        printf("Error while receiving server's msg\n");
    } else if (strcmp(serverM_message, "ALL input names are not existed.") == 0) {
        printf("Client received the reply from Main Server using TCP over port %i:\n%s\n",
               ntohs(client_addr.sin_port), serverM_message);
        return -1;
    } else {
        printf("Client received the reply from Main Server using TCP over port %i:\nTime intervals %s.\n",
               ntohs(client_addr.sin_port), serverM_message);
        if (serverM_message[1] == ']') {
            return -1;
        }
    }

    return 0;
}

void update() {
    decode_info(serverM_message);
    // Get input time from user
    printf("Please enter the final meeting time to register an meeting:\n");
    fgets(input, sizeof(input), stdin);
    while (check_input(input) < 0) {
        printf("Time interval %s is not valid. Please enter again:\n", strtok(input, "\n"));
        fgets(input, sizeof(input), stdin);
    }

    // Send update info to serverM
    if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
        printf("Unable to send message to serverM\n");
    } else {
        printf("Sent the request to register %s as the meeting time for %s.\n", strtok(input, "\n"), names);
    }

    // Receive the final update
    if (recv(socket_desc, serverM_message, sizeof(serverM_message), 0) < 0) {
        printf("Error while receiving server's msg\n");
    } else {
        printf("Register a meeting at %s and update the availability for the following users:\n%s",
               input, serverM_message);
    }
}

void init_slots() {
    for (int i = 0; i < 51; i++) {
        slots[i][0] = -1;
        slots[i][1] = -1;
    }
}

void decode_info(char message[4000]) {
    init_slots();

    char tmp_str[4000];
    strcpy(tmp_str, message);

    // Decode time slots
    char *time_str = strtok(tmp_str, " ");
    int slot_idx = 0;
    int start = 0;
    // Remove the '[]' for time_str
    time_str++;
    time_str[strlen(time_str) - 1] = 0;
    // Get each time slot
    char *time = strtok(time_str, ",[]");
    while (time != NULL) {
        slots[slot_idx][start] = atoi(time);
        if (start == 1) {
            slot_idx++;
        }
        start = 1 - start;
        time = strtok(NULL, ",[]");
    }
    free(time);

    strcpy(tmp_str, message);
    char *name_str = strtok(tmp_str, " ");
    name_str = strtok(NULL, " ");
    name_str = strtok(NULL, " ");
    name_str = strtok(NULL, ",");
    while (name_str != NULL) {
        strcat(names, name_str);
        strcat(names, ",");
        name_str = strtok(NULL, ",");
    }
    names[strlen(names) - 1] = 0;
}

int check_input(char input_slot[4000]) {
    char tmp_str[4000];
    strcpy(tmp_str, input_slot);

    // Decode input string
    char *time_str = strtok(tmp_str, "\n");
    // Remove the '[]' for time_str
    time_str++;
    time_str[strlen(time_str) - 1] = 0;
    // Get start time and end time from the input
    int start_time = atoi(strtok(time_str, ","));
    int end_time = atoi(strtok(NULL, ","));

    if (start_time >= end_time) {
        return -1;
    }

    // Check validation
    int i = 0;
    while (slots[i][0] != -1) {
        if (start_time >= slots[i][0] && end_time <= slots[i][1]) {
            // save info in to client_message
            char str[5];
            strcpy(client_message, "");
            sprintf(str, "%d", start_time);
            strcat(client_message, str);
            strcat(client_message, ",");
            sprintf(str, "%d", end_time);
            strcat(client_message, str);
            return 1;
        }
        i++;
    }
    return -1;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Socket-related
int socket_desc_UDP,
    socket_desc_TCP,
    client_sock;

struct sockaddr_in serverM_addr_UDP,
                   serverM_addr_TCP,
                   serverA_addr,
                   serverB_addr,
                   client_addr;

char serverM_message[4000],
     serverA_message[4000],
     serverB_message[4000],
     client_message[4000];

socklen_t serverA_struct_length = sizeof(serverA_addr);
socklen_t serverB_struct_length = sizeof(serverB_addr);
socklen_t client_size = sizeof(client_addr);

// Schedule info
char names_A[4000];
char names_B[4000];
char names_A_list[200][20];
char names_B_list[200][20];
int idx_A;
int idx_B;
char names_client_A[4000];
char names_client_B[4000];
char names_client[4000];
char result_str_A[4000];
char result_str_B[4000];
int result_A[51][2];
int result_B[51][2];
int result[51][2];

void clean_message_buffers();
void setup_UDP();
void setup_TCP();
void get_names();
int calculate_intervals();
void update();

// helper functions
void check_names_client();
void calculate();
void init_result();
void decode_time(char result_A_str[4000], char result_B_str[4000]);

int main() {
    // Clean message buffers
    clean_message_buffers();

    // Setup UDP socket
    setup_UDP();

    // Setup TCP socket
    setup_TCP();

    // Receive and forward msg
    while (1) {
        // Get and check names from client's input
        get_names();

        // Get calculated intervals from backend servers,
        // calculate final intervals based on backend servers' results
        int result = calculate_intervals();

        // Update:
        // step 1: Get update info from client's input,
        // step 2: forward update info to backend servers,
        // step 3: get updated results from backend servers,
        // step 4: forward updated results to client
        if (result != -1) {
            update();
        }

        clean_message_buffers();
    }

    // Close the sockets
    close(socket_desc_UDP);
    close(client_sock);
    close(socket_desc_TCP);

    return 0;
}

void clean_message_buffers() {
    memset(serverM_message, '\0', sizeof(serverM_message));
    memset(serverA_message, '\0', sizeof(serverA_message));
    memset(serverB_message, '\0', sizeof(serverB_message));
    memset(client_message, '\0', sizeof(client_message));

    memset(names_client_A, '\0', sizeof(names_client_A));
    memset(names_client_B, '\0', sizeof(names_client_B));
    memset(names_client, '\0', sizeof(names_client));
    memset(result_str_A, '\0', sizeof(result_str_A));
    memset(result_str_B, '\0', sizeof(result_str_B));
}

void setup_UDP() {
    // Set port and IP for UDP connections
    serverM_addr_UDP.sin_family = AF_INET;
    serverM_addr_UDP.sin_port = htons(23941);
    serverM_addr_UDP.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Create UDP socket
    socket_desc_UDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Bind to the set port and IP for UDP connections
    if (bind(socket_desc_UDP, (struct sockaddr*)&serverM_addr_UDP, sizeof(serverM_addr_UDP)) < 0) {
        printf("Couldn't bind to the port\n");
    }

    printf("%s\n", "Main Server is up and running.");

    memset(names_A, '\0', sizeof(names_A));
    memset(names_B, '\0', sizeof(names_B));

    char tmp_names[4000];
    char *name;
    // Receive serverA's names
    if (recvfrom(socket_desc_UDP, names_A, sizeof(names_A), 0,
                 (struct sockaddr*)&serverA_addr, &serverA_struct_length) < 0) {
        printf("Couldn't receive from A\n");
    }
    printf("Main Server received the username list from server A using UDP over port %i.\n",
           ntohs(serverM_addr_UDP.sin_port));
    strcpy(tmp_names, names_A);
    name = strtok(tmp_names, " ");
    idx_A = 0;
    while (name != NULL) {
        strcpy(names_A_list[idx_A], name);
        name = strtok(NULL, " ");
        idx_A++;
    }

    // Receive serverB's names
    if (recvfrom(socket_desc_UDP, names_B, sizeof(names_B), 0,
                 (struct sockaddr*)&serverB_addr, &serverB_struct_length) < 0) {
        printf("Couldn't receive from B\n");
    }
    printf("Main Server received the username list from server B using UDP over port %i.\n",
           ntohs(serverM_addr_UDP.sin_port));
    strcpy(tmp_names, names_B);
    name = strtok(tmp_names, " ");
    idx_B = 0;
    while (name != NULL) {
        strcpy(names_B_list[idx_B], name);
        name = strtok(NULL, " ");
        idx_B++;
    }
}

void setup_TCP() {
    // Set port and IP for TCP connection
    serverM_addr_TCP.sin_family = AF_INET;
    serverM_addr_TCP.sin_port = htons(24941);
    serverM_addr_TCP.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Create TCP socket
    socket_desc_TCP = socket(AF_INET, SOCK_STREAM, 0);

    // Bind to the set port and IP for TCP connection
    if (bind(socket_desc_TCP, (struct sockaddr*)&serverM_addr_TCP, sizeof(serverM_addr_TCP)) < 0) {
        printf("Couldn't bind to the port\n");
    }

    // Listen for clients
    if (listen(socket_desc_TCP, 1) < 0) {
        printf("Error listening connection\n");
    }

    // Accept an incoming connection
    client_size = sizeof(client_addr);
    client_sock = accept(socket_desc_TCP, (struct sockaddr*)&client_addr, &client_size);
    if (client_sock < 0) {
        printf("Can't accept connection\n");
    }
}

void get_names() {
    // Receive client's names
    if (recv(client_sock, client_message, sizeof(client_message), 0) < 0) {
        printf("Couldn't receive from client\n");
    } else {
        printf("Main Server received the request from client using TCP over port %i.\n",
               ntohs(serverM_addr_TCP.sin_port));
    }

    // Check and save names from client's input
    check_names_client();

    // Forward names to backend servers
    strcpy(result_str_A, "NO RETURN");
    strcpy(result_str_B, "NO RETURN");
    if (strcmp(names_client_A, "") != 0) {
        if (sendto(socket_desc_UDP, names_client_A, strlen(names_client_A), 0,
                   (struct sockaddr*)&serverA_addr, serverA_struct_length) < 0) {
            printf("Can't send to A\n");
        }
        printf("Found %s located at Server A. Send to Server A.\n",
               names_client_A);

        // Receive serverA's result
        memset(result_str_A, '\0', sizeof(result_str_A));
        if (recvfrom(socket_desc_UDP, result_str_A, sizeof(result_str_A), 0,
                     (struct sockaddr*)&serverA_addr, &serverA_struct_length) < 0) {
            printf("Couldn't receive from A\n");
        }
        printf("Main Server received from server A the intersection result using UDP over port %i:\n[%s].\n",
                   ntohs(serverM_addr_UDP.sin_port), result_str_A);
    }

    if (strcmp(names_client_B, "") != 0) {
        if (sendto(socket_desc_UDP, names_client_B, strlen(names_client_B), 0,
                   (struct sockaddr*)&serverB_addr, serverB_struct_length) < 0) {
            printf("Can't send to B\n");
        }
        printf("Found %s located at Server B. Send to Server B.\n",
               names_client_B);

        // Receive serverB's result
        memset(result_str_B, '\0', sizeof(result_str_B));
        if (recvfrom(socket_desc_UDP, result_str_B, sizeof(result_str_B), 0,
                     (struct sockaddr*)&serverB_addr, &serverB_struct_length) < 0) {
            printf("Couldn't receive from B\n");
        }
        printf("Main Server received from server B the intersection result using UDP over port %i:\n[%s].\n",
                   ntohs(serverM_addr_UDP.sin_port), result_str_B);
    }
}

void check_names_client() {
    char *name;

    memset(serverM_message, '\0', sizeof(serverM_message));
    memset(names_client, '\0', sizeof(names_client));
    name = strtok(client_message, " \n");
    while (name != NULL) {
        int exist = -1;

        for (int i = 0; i < idx_A; i++) {
            if (strcmp(names_A_list[i], name) == 0) {
                strcat(names_client_A, name);
                strcat(names_client_A, ", ");
                strcat(names_client, name);
                strcat(names_client, ", ");
                exist = 1;
                break;
            }
        }

        for (int i = 0; i < idx_B; i++) {
            if (strcmp(names_B_list[i], name) == 0) {
                strcat(names_client_B, name);
                strcat(names_client_B, ", ");
                strcat(names_client, name);
                strcat(names_client, ", ");
                exist = 1;
                break;
            }
        }

        if (exist == -1) {
            strcat(serverM_message, name);
            strcat(serverM_message, ", ");
        }

        name = strtok(NULL, " \n");
    }
    free(name);

    if (strcmp(names_client_A, "") != 0) {
        names_client_A[strlen(names_client_A) - 1] = 0;
        names_client_A[strlen(names_client_A) - 1] = 0;
    }

    if (strcmp(names_client_B, "") != 0) {
        names_client_B[strlen(names_client_B) - 1] = 0;
        names_client_B[strlen(names_client_B) - 1] = 0;
    }

    if (strcmp(names_client, "") != 0) {
        names_client[strlen(names_client) - 1] = 0;
        names_client[strlen(names_client) - 1] = 0;
    }

    if (strcmp(serverM_message, "") != 0) {
        serverM_message[strlen(serverM_message) - 1] = 0;
        serverM_message[strlen(serverM_message) - 1] = 0;
        strcat(serverM_message, " do not exist.");
    } else {
        strcpy(serverM_message, "all exist");
    }

    // Forward non-existed names to client
    if (send(client_sock, serverM_message, strlen(serverM_message), 0) < 0) {
        printf("Can't send to client\n");
    } else if (strcmp(serverM_message, "all exist") != 0) {
        printf("%s Send a reply to the client.\n", serverM_message);
    }
}

int calculate_intervals() {
    // All input names are not existed
    if (strcmp(names_client, "") == 0) {
        sleep(1);
        if (send(client_sock, "ALL input names are not existed.", strlen("ALL input names are not existed."), 0) < 0) {
            printf("Can't send to client\n");
        } else {
            printf("Main Server sent the result to the client.\n");
        }
        return -1;
    }

    // Calculate the final result
    if (strcmp(result_str_A, "NO RETURN") == 0) {
        strcpy(serverM_message, "[");
        strcat(serverM_message, result_str_B);
        strcat(serverM_message, "]");
        printf("Found the intersection between the results from server A and B:\n%s.\n", serverM_message);
    } else if (strcmp(result_str_B, "NO RETURN") == 0) {
        strcpy(serverM_message, "[");
        strcat(serverM_message, result_str_A);
        strcat(serverM_message, "]");
        printf("Found the intersection between the results from server A and B:\n%s.\n", serverM_message);
    } else {
        calculate();
    }

    // Send result to client
    sleep(1);
    strcat(serverM_message, " works for ");
    strcat(serverM_message, names_client);
    if (send(client_sock, serverM_message, strlen(serverM_message), 0) < 0) {
        printf("Can't send to client\n");
    } else {
        printf("Main Server sent the result to the client.\n");
    }
    if (serverM_message[1] == ']') {
        // Forward no need to update info to backend servers
        if (strcmp(names_client_A, "") != 0) {
            if (sendto(socket_desc_UDP, "No need to update", strlen("No need to update"), 0,
                       (struct sockaddr*)&serverA_addr, serverA_struct_length) < 0) {
                printf("Can't send to A\n");
            }
        }
        if (strcmp(names_client_B, "") != 0) {
            if (sendto(socket_desc_UDP, "No need to update", strlen("No need to update"), 0,
                       (struct sockaddr*)&serverB_addr, serverB_struct_length) < 0) {
                printf("Can't send to B\n");
            }
        }
        return -1;
    }

    return 0;
}

void calculate() {
    init_result();
    decode_time(result_str_A, result_str_B);

    // The algorithm
    int result_A_ptr = 0;
    int result_B_ptr = 0;
    int result_ptr = 0;
    while (result_A[result_A_ptr][0] != -1 && result_B[result_B_ptr][0] != -1) {
        if (result_A[result_A_ptr][1] <= result_B[result_B_ptr][0]) {
            result_A_ptr++;
            continue;
        }
        if (result_B[result_B_ptr][1] <= result_A[result_A_ptr][0]) {
            result_B_ptr++;
            continue;
        }

        int start_time;
        if (result_A[result_A_ptr][0] >= result_B[result_B_ptr][0]) {
            start_time = result_A[result_A_ptr][0];
        } else {
            start_time = result_B[result_B_ptr][0];
        }

        int end_time;
        if (result_A[result_A_ptr][1] <= result_B[result_B_ptr][1]) {
            end_time = result_A[result_A_ptr][1];
            result_A_ptr++;
        } else {
            end_time = result_B[result_B_ptr][1];
            result_B_ptr++;
        }

        result[result_ptr][0] = start_time;
        result[result_ptr][1] = end_time;
        result_ptr++;
    }

    // Save final result to serverM_message
    memset(serverM_message, '\0', sizeof(serverM_message));
    strcpy(serverM_message, "[");
    int i = 0;
    char str[5];
    while (result[i][0] != -1) {
        strcat(serverM_message, "[");
        sprintf(str, "%d", result[i][0]);
        strcat(serverM_message, str);
        strcat(serverM_message, ",");
        sprintf(str, "%d", result[i][1]);
        strcat(serverM_message, str);
        strcat(serverM_message, "],");
        i++;
    }
    if (i != 0) {
        serverM_message[strlen(serverM_message) - 1] = 0;
    }
    strcat(serverM_message, "]");
    printf("Found the intersection between the results from server A and B:\n%s.\n", serverM_message);
}

void decode_time(char result_A_str[4000], char result_B_str[4000]) {
    int start;
    int i;
    char *time;

    // Get each time slot from A
    start = 0;
    i = 0;
    time = strtok(result_A_str, ",[]");
    while (time != NULL) {
        result_A[i][start] = atoi(time);
        if (start == 1) {
            i++;
        }
        start = 1 - start;
        time = strtok(NULL, ",[]");
    }

    // Get each time slot from B
    start = 0;
    i = 0;
    time = strtok(result_B_str, ",[]");
    while (time != NULL) {
        result_B[i][start] = atoi(time);
        if (start == 1) {
            i++;
        }
        start = 1 - start;
        time = strtok(NULL, ",[]");
    }

    free(time);
}

void init_result() {
    for (int i = 0; i < 51; i++) {
        result_A[i][0] = -1;
        result_A[i][1] = -1;
        result_B[i][0] = -1;
        result_B[i][1] = -1;
        result[i][0] = -1;
        result[i][1] = -1;
    }
}

void update() {
    // Receive client's update interval info
    if (recv(client_sock, client_message, sizeof(client_message), 0) < 0) {
        printf("Couldn't receive from client\n");
    }

    // Forward update info to backend servers
    strcpy(serverA_message, "");
    strcpy(serverB_message, "");
    if (strcmp(names_client_A, "") != 0) {
        strcpy(serverM_message, "");
        strcat(serverM_message, names_client_A);
        strcat(serverM_message, ";");
        strcat(serverM_message, client_message);
        if (sendto(socket_desc_UDP, serverM_message, strlen(serverM_message), 0,
                   (struct sockaddr*)&serverA_addr, serverA_struct_length) < 0) {
            printf("Can't send to A\n");
        }

        // Receive serverA's update result
        if (recvfrom(socket_desc_UDP, serverA_message, sizeof(serverA_message), 0,
                     (struct sockaddr*)&serverA_addr, &serverA_struct_length) < 0) {
            printf("Couldn't receive from A\n");
        }
    }

    if (strcmp(names_client_B, "") != 0) {
        strcpy(serverM_message, "");
        strcat(serverM_message, names_client_B);
        strcat(serverM_message, ";");
        strcat(serverM_message, client_message);
        if (sendto(socket_desc_UDP, serverM_message, strlen(serverM_message), 0,
                   (struct sockaddr*)&serverB_addr, serverB_struct_length) < 0) {
            printf("Can't send to B\n");
        }

        // Receive serverB's update result
        if (recvfrom(socket_desc_UDP, serverB_message, sizeof(serverB_message), 0,
                     (struct sockaddr*)&serverB_addr, &serverB_struct_length) < 0) {
            printf("Couldn't receive from B\n");
        }
    }

    // Send update result to client
    strcpy(serverM_message, "");
    strcat(serverM_message, serverA_message);
    strcat(serverM_message, serverB_message);
    if (send(client_sock, serverM_message, strlen(serverM_message), 0) < 0) {
        printf("Can't send to client\n");
    } else {
        printf("Main Server sent the update result to the client.\n");
    }
}
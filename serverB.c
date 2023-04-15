#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Socket-related
int socket_desc;
struct sockaddr_in serverM_addr, serverB_addr;
char serverM_message[4000], serverB_message[4000];
socklen_t serverM_struct_length = sizeof(serverM_addr);

// Schedule info
char names[200][20];
int slots[200][51][2];
int idx;

void read_in_info();
void clean_message_buffers();
void setup_UDP();
void calculate_intervals();
void update();

// helper functions
void decode_time(char *time_str, int index);
int get_idx(char *name);
void init_slots();
void calculate();
void remove_space(char *str);

int main() {
    // Read from b.txt
    read_in_info();

    // Clean message buffers
    clean_message_buffers();

    // Setup UDP socket
    setup_UDP();

    // Receive and forward msg
    while (1) {
        calculate_intervals();
        update();
        clean_message_buffers();
    }

    // Close the socket
    close(socket_desc);

    return 0;
}

void read_in_info() {
    FILE *fp;
    char info[4000];
    idx = 0;

    init_slots();

    fp = fopen("b.txt", "r");
    if (fp == NULL) {
        perror("Error opening file B");
    }
    while (fgets(info, 4000, fp) != NULL) {
        char *name = strtok(info, ";");
        remove_space(name);
        strcpy(names[idx], name);
        decode_time(strtok(NULL, ";"), idx);
        idx++;
    }
    fclose(fp);
}

void remove_space(char *str) {
    char *tmp = str;
    do {
        while (*tmp == ' ') {
            ++tmp;
        }
    } while ((*str++ = *tmp++));
}

void init_slots() {
    for (int i = 0; i < 200; i++) {
        for (int j = 0; j < 51; j++) {
            slots[i][j][0] = -1;
            slots[i][j][1] = -1;
        }
    }
}

void decode_time(char *time_str, int index) {
    int slot_idx = 0;
    int start = 0;

    // Remove the '[]' for time_str
    time_str++;
    time_str[strlen(time_str) - 1] = 0;
    time_str[strlen(time_str) - 1] = 0;

    // Get each time slot
    char *time = strtok(time_str, ",[] ");
    while (time != NULL) {
        printf("'%s'\n", time);
        slots[index][slot_idx][start] = atoi(time);
        if (start == 1) {
            slot_idx++;
        }
        start = 1 - start;
        time = strtok(NULL, ",[] ");
    }

    free(time);
}

void clean_message_buffers() {
    memset(serverM_message, '\0', sizeof(serverM_message));
    memset(serverB_message, '\0', sizeof(serverB_message));
}

void setup_UDP() {
    // Set port and IP
    serverM_addr.sin_family = AF_INET;
    serverM_addr.sin_port = htons(23941);
    serverM_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    serverB_addr.sin_family = AF_INET;
    serverB_addr.sin_port = htons(22941);
    serverB_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Create socket
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Bind to the set port and IP
    if (bind(socket_desc, (struct sockaddr*)&serverB_addr, sizeof(serverB_addr)) < 0) {
        printf("Couldn't bind to the port\n");
    }

    printf("Server B is up and running using UDP on port %i.\n",
           ntohs(serverB_addr.sin_port));

    // Send names in serverB to serverM
    for (int i = 0; i < idx; i++) {
        strcat(serverB_message, names[i]);
        strcat(serverB_message, " ");
    }
    if (sendto(socket_desc, serverB_message, strlen(serverB_message), 0,
               (struct sockaddr*)&serverM_addr, serverM_struct_length) < 0) {
        printf("Unable to send names\n");
    } else {
        printf("Server B finished sending a list of usernames to Main Server.\n");
    }
}

void calculate_intervals() {
    // Receive serverM's response
    if (recvfrom(socket_desc, serverM_message, sizeof(serverM_message), 0,
                 (struct sockaddr*)&serverM_addr, &serverM_struct_length) < 0) {
        printf("Error while receiving serverM's msg\n");
    } else {
        printf("Server B received the usernames from Main Server using UDP over port %i.\n",
               ntohs(serverB_addr.sin_port));
    }

    // Calculate intervals
    if (strcmp(serverM_message, "") != 0) {
        calculate();
    } else {
        strcpy(serverB_message, "");
    }

    // Send result to server M
    if (sendto(socket_desc, serverB_message, strlen(serverB_message), 0,
               (struct sockaddr*)&serverM_addr, serverM_struct_length) < 0) {
        printf("Unable to send names\n");
    } else {
        printf("Server B finished sending the response to Main Server.\n");
    }
}

void calculate() {
    char tmp_names[4000];
    strcpy(tmp_names, serverM_message);

    // step 1: initialize result buffer
    int result[51][2];
    result[0][0] = 0;
    result[0][1] = 100;
    for (int i = 1; i < 51; i++) {
        result[i][0] = -1;
        result[i][1] = -1;
    }

    // step 2: the algorithm
    char *name = strtok(tmp_names, ", ");
    while (name != NULL) {
        int name_idx = get_idx(name);

        int result_ptr = 0;
        int slot_ptr = 0;
        int tmp_ptr = 0;
        int tmp_result[51][2];
        for (int i = 0; i < 51; i++) {
            tmp_result[i][0] = -1;
            tmp_result[i][1] = -1;
        }
        while (result[result_ptr][0] != -1 && slots[name_idx][slot_ptr][0] != -1) {
            if (result[result_ptr][1] <= slots[name_idx][slot_ptr][0]) {
                result_ptr++;
                continue;
            }
            if (slots[name_idx][slot_ptr][1] <= result[result_ptr][0]) {
                slot_ptr++;
                continue;
            }

            int start_time;
            if (result[result_ptr][0] >= slots[name_idx][slot_ptr][0]) {
                start_time = result[result_ptr][0];
            } else {
                start_time = slots[name_idx][slot_ptr][0];
            }

            int end_time;
            if (result[result_ptr][1] <= slots[name_idx][slot_ptr][1]) {
                end_time = result[result_ptr][1];
                result_ptr++;
            } else {
                end_time = slots[name_idx][slot_ptr][1];
                slot_ptr++;
            }

            tmp_result[tmp_ptr][0] = start_time;
            tmp_result[tmp_ptr][1] = end_time;
            tmp_ptr++;
        }

        // Assign tmp_result into result
        for (int i = 0; i < 51; i++) {
            result[i][0] = tmp_result[i][0];
            result[i][1] = tmp_result[i][1];
        }

        name = strtok(NULL, " ");
    }

    // step 3: save calculated result to serverA_message
    strcpy(serverB_message, "");
    int i = 0;
    char str[5];
    while (result[i][0] != -1) {
        strcat(serverB_message, "[");
        sprintf(str, "%d", result[i][0]);
        strcat(serverB_message, str);
        strcat(serverB_message, ",");
        sprintf(str, "%d", result[i][1]);
        strcat(serverB_message, str);
        strcat(serverB_message, "],");
        i++;
    }
    serverB_message[strlen(serverB_message) - 1] = 0;
    printf("Found the intersection result: [%s] for %s.\n", serverB_message, serverM_message);
}

int get_idx(char *name) {
    for (int i = 0; i < idx; i++) {
        if (strcmp(name, names[i]) == 0) {
            return i;
        }
    }
}

void update() {

}
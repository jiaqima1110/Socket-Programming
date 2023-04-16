#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Socket-related
int socket_desc;
struct sockaddr_in serverM_addr, serverA_addr;
char serverM_message[4000], serverA_message[4000];
socklen_t serverM_struct_length = sizeof(serverM_addr);

// Schedule info
char names[200][20];
int slots[200][52][2];
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
void update_slot(int slot_idx, int start, int end);

int main() {
    // Read from a.txt
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

    fp = fopen("a.txt", "r");
    if (fp == NULL) {
        perror("Error opening file A");
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
        for (int j = 0; j < 52; j++) {
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
    memset(serverA_message, '\0', sizeof(serverA_message));
}

void setup_UDP() {
    // Set port and IP
    serverM_addr.sin_family = AF_INET;
    serverM_addr.sin_port = htons(23941);
    serverM_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    serverA_addr.sin_family = AF_INET;
    serverA_addr.sin_port = htons(21941);
    serverA_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Create socket
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Bind to the set port and IP
    if (bind(socket_desc, (struct sockaddr*)&serverA_addr, sizeof(serverA_addr)) < 0) {
        printf("Couldn't bind to the port\n");
    }

    printf("Server A is up and running using UDP on port %i.\n",
           ntohs(serverA_addr.sin_port));

    // Send names in server A to server M
    for (int i = 0; i < idx; i++) {
        strcat(serverA_message, names[i]);
        strcat(serverA_message, " ");
    }
    if (sendto(socket_desc, serverA_message, strlen(serverA_message), 0,
               (struct sockaddr*)&serverM_addr, serverM_struct_length) < 0) {
        printf("Unable to send names\n");
    } else {
        printf("Server A finished sending a list of usernames to Main Server.\n");
    }
}

void calculate_intervals() {
    // Receive serverM's response
    if (recvfrom(socket_desc, serverM_message, sizeof(serverM_message), 0,
                 (struct sockaddr*)&serverM_addr, &serverM_struct_length) < 0) {
        printf("Error while receiving serverM's msg\n");
    } else {
        printf("Server A received the usernames from Main Server using UDP over port %i.\n",
               ntohs(serverA_addr.sin_port));
    }

    // Calculate intervals
    if (strcmp(serverM_message, "") != 0) {
        calculate();
    } else {
        strcpy(serverA_message, "");
    }

    // Send result to server M
    if (sendto(socket_desc, serverA_message, strlen(serverA_message), 0,
               (struct sockaddr*)&serverM_addr, serverM_struct_length) < 0) {
        printf("Unable to send results\n");
    } else {
        printf("Server A finished sending the response to Main Server.\n");
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
    strcpy(serverA_message, "");
    int i = 0;
    char str[5];
    while (result[i][0] != -1) {
        strcat(serverA_message, "[");
        sprintf(str, "%d", result[i][0]);
        strcat(serverA_message, str);
        strcat(serverA_message, ",");
        sprintf(str, "%d", result[i][1]);
        strcat(serverA_message, str);
        strcat(serverA_message, "],");
        i++;
    }
    serverA_message[strlen(serverA_message) - 1] = 0;
    printf("Found the intersection result: [%s] for %s.\n", serverA_message, serverM_message);
}

int get_idx(char *name) {
    for (int i = 0; i < idx; i++) {
        if (strcmp(name, names[i]) == 0) {
            return i;
        }
    }
    return 0;
}

void update() {
    // Receive serverM's update info
    if (recvfrom(socket_desc, serverM_message, sizeof(serverM_message), 0,
                 (struct sockaddr*)&serverM_addr, &serverM_struct_length) < 0) {
        printf("Error while receiving serverM's msg\n");
    }

    strcpy(serverA_message, "");
    char tmp_str[4000];
    char str[4000];

    // Decode start time and end time
    strcpy(tmp_str, serverM_message);
    char *time_str = strtok(tmp_str, ";");
    time_str = strtok(NULL, ";");
    int start_time = atoi(strtok(time_str, ","));
    int end_time = atoi(strtok(NULL, ","));
    printf("Register a meeting at [%d,%d] and update the availability for the following users:\n",
            start_time, end_time);

    // Update each time slot
    strcpy(tmp_str, serverM_message);
    char *name_str = strtok(tmp_str, ";");
    name_str = strtok(name_str, ", ");
    while (name_str != NULL) {
        sprintf(str, "%s: updated from [", name_str);
        strcat(serverA_message, str);
        int name_idx = get_idx(name_str);
        update_slot(name_idx, start_time, end_time);
        name_str = strtok(NULL, ", ");
    }

    // Send update to server M
    printf("%s", serverA_message);
    if (sendto(socket_desc, serverA_message, strlen(serverA_message), 0,
               (struct sockaddr*)&serverM_addr, serverM_struct_length) < 0) {
        printf("Unable to send results\n");
    } else {
        printf("Notified Main Server that registration has finished.\n");
    }
}

void update_slot(int slot_idx, int start, int end) {
    char str[4000];

    // Origin time slots
    int p = 0;
    while (slots[slot_idx][p][0] != -1) {
        sprintf(str, "[%d,%d]", slots[slot_idx][p][0], slots[slot_idx][p][1]);
        strcat(serverA_message, str);
        if (slots[slot_idx][p+1][0] != -1) {
            strcat(serverA_message, ",");
        }
        p++;
    }
    strcat(serverA_message, "] to [");

    // Find the slot need to be modified
    int i = 0;
    while (slots[slot_idx][i][0] != -1) {
        if (slots[slot_idx][i][0] <= start && slots[slot_idx][i][1] >= end) {
            // Found the slot need to be modified
            break;
        }
        i++;
    }

    // Modify the slot
    if (slots[slot_idx][i][0] == start && slots[slot_idx][i][1] > end) {
        slots[slot_idx][i][0] = end;
    } else if (slots[slot_idx][i][0] < start && slots[slot_idx][i][1] == end) {
        slots[slot_idx][i][1] = start;
    } else if (slots[slot_idx][i][0] == start && slots[slot_idx][i][1] == end) {
        // Remove the slot
        int j = i;
        while (slots[slot_idx][j][0] != -1) {
            slots[slot_idx][j][0] = slots[slot_idx][j+1][0];
            slots[slot_idx][j][1] = slots[slot_idx][j+1][1];
            j++;
        }
    } else {
        // Break the slot
        int j = i;
        while (slots[slot_idx][j][0] != -1) {
            j++;
        }
        while (j != i) {
            slots[slot_idx][j][0] = slots[slot_idx][j-1][0];
            slots[slot_idx][j][1] = slots[slot_idx][j-1][1];
            j--;
        }
        slots[slot_idx][i][1] = start;
        slots[slot_idx][i+1][0] = end;
    }

    // Updated time slots
    int q = 0;
    while (slots[slot_idx][q][0] != -1) {
        sprintf(str, "[%d,%d]", slots[slot_idx][q][0], slots[slot_idx][q][1]);
        strcat(serverA_message, str);
        if (slots[slot_idx][q+1][0] != -1) {
            strcat(serverA_message, ",");
        }
        q++;
    }
    strcat(serverA_message, "]\n");
}

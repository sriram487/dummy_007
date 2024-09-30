#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE (6 * 1024)

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    FILE *file;
    const char *directory = "./data/sample_2/"; // Directory where the CSV files are located

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Specify server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change to server IP if needed

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Open the directory
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        perror("Could not open directory");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Only process CSV files
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".csv")) {
            // Wait for the "READY" message from the server
            ssize_t bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0'; // Null-terminate the string
                printf("Received: %s", buffer);

                if (strcmp(buffer, "READY\n") == 0) {
                    // Construct the full file path
                    char full_path[256];
                    snprintf(full_path, sizeof(full_path), "%s%s", directory, entry->d_name);

                    // Open the CSV file
                    file = fopen(full_path, "r");
                    if (file == NULL) {
                        perror("File opening failed");
                        continue; // Skip to the next file
                    }

                    // Allocate a buffer for the entire content
                    size_t total_length = 0;
                    char *file_content = malloc(BUFFER_SIZE);
                    if (file_content == NULL) {
                        perror("Memory allocation failed");
                        fclose(file);
                        continue;
                    }

                    // Send start delimiter
                    total_length += snprintf(file_content + total_length, BUFFER_SIZE - total_length, "$\n");

                    // Read the CSV file content into the allocated buffer
                    while (fgets(buffer, BUFFER_SIZE, file)) {
                        if (total_length + strlen(buffer) < BUFFER_SIZE) {
                            strcpy(file_content + total_length, buffer);
                            total_length += strlen(buffer);
                        } else {
                            printf("Buffer overflow, skipping remaining data for file '%s'.\n", entry->d_name);
                            break; // Stop if the buffer is full
                        }
                    }

                    // Send end delimiter
                    total_length += snprintf(file_content + total_length, BUFFER_SIZE - total_length, "&\n");

                    // Send the entire content at once
                    send(sock, file_content, total_length, 0);
                    fclose(file);
                    free(file_content);
                    printf("CSV file '%s' sent with delimiters.\n", entry->d_name);

                    // Wait for acknowledgment
                    bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
                    if (bytes_received > 0) {
                        buffer[bytes_received] = '\0'; // Null-terminate the string
                        printf("Received: %s", buffer);
                    }
                }
            }
        }
    }

    closedir(dir); // Close the directory
    close(sock); // Close the socket
    return 0;
}

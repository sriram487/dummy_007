#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <opencv2/opencv.hpp>

#define PORT 8080
#define BUFFER_SIZE (6 * 1024)
#define MAX_FLOATS (118 * 3)

void draw_star(int x, int y, float magnitude, cv::Mat* background, int ROI) {
    double H = 90000 * exp(-magnitude + 1);
    double sigma = 0.5;

    for (int u = x - ROI; u <= x + ROI; u++) {
        for (int v = y - ROI; v <= y + ROI; v++) {
            double dist = (u - x) * (u - x) + (v - y) * (v - y);
            double diff = dist / (2 * (sigma * sigma));
            double exponent_exp = exp(-diff);
            int raw_intensity = (int)round((H / (2 * CV_PI * (sigma * sigma))) * exponent_exp);

            if (u >= 0 && u < background->cols && v >= 0 && v < background->rows) {
                (*background).at<uchar>(v, u) = (uchar)fmin(raw_intensity, 255);
            }
        }
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    char full_data[BUFFER_SIZE] = {0}; // Buffer to hold all received data
    ssize_t bytes_received;

    // Create a background image
    cv::Mat background = cv::Mat::zeros(1400, 1400, CV_8UC1);
    cv::namedWindow("Star Image", cv::WINDOW_NORMAL);

    // Continuously send "READY" message
    while (1) {
        const char *message = "READY\n";
        send(client_socket, message, strlen(message), 0);
        printf("Sent: %s", message);

        // Receive data from the client
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Error or connection closed\n");
            break; // Error or connection closed
        }
        buffer[bytes_received] = '\0'; // Null-terminate the string

        printf("Received %ld bytes\n", bytes_received);

        // Check for start and end delimiters
        if (buffer[0] == '$' && strchr(buffer, '&') != NULL) {

            // Process the data
            char *end_ptr = strchr(buffer, '&');
            *end_ptr = '\0'; // Remove the end delimiter

            // Append the received buffer to full_data
            strncat(full_data, buffer + 1, sizeof(full_data) - strlen(full_data) - 1); // Skip the '$'

            // Now parse the full_data into float_array
            float float_array[MAX_FLOATS];
            size_t float_count = 0;

            for (size_t i = 0; i < strlen(full_data); i++) {
                if (full_data[i] == '\n') {
                    full_data[i] = ','; // Replace newline with comma
                }
            }

            char *token = strtok(full_data, ",");
            while (token != NULL && float_count < MAX_FLOATS) {
                float_array[float_count++] = strtof(token, NULL); // Convert to float
                token = strtok(NULL, ",");
            }

            // Draw stars using batches of 3 values
            for (size_t i = 0; i < float_count; i += 3) {
                if (i + 2 < float_count) { // Check if there are enough floats  this take 0.6ms 
                    int x = (int)float_array[i];
                    int y = (int)float_array[i + 1];
                    float magnitude = float_array[i + 2];
                    draw_star(x, y, magnitude, &background, 5);
                }
            }

            auto start_time = std::chrono::high_resolution_clock::now(); // Start timing

            // Display the image using OpenCV
            cv::imshow("Star Image", background);
            cv::waitKey(1); // Allow the window to refresh

            auto end_time = std::chrono::high_resolution_clock::now(); // End timing
            std::chrono::duration<double> duration = end_time - start_time;
            std::cout << "Time taken to process and display images: " << duration.count() << " seconds." << std::endl;

            // Send acknowledgment back to the client
            const char *ack = "ACK: RD.\n";
            send(client_socket, ack, strlen(ack), 0);
            printf("Sent: %s", ack);

        } 
        
        else {
            // Send another acknowledgment to the client if delimiters are not present
            const char *ack = "ACK: Waiting for proper data format.\n";
            send(client_socket, ack, strlen(ack), 0);
            printf("Sent: %s", ack);
        }

        // Reset for the next iteration
        memset(full_data, 0, sizeof(full_data));
        
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    listen(server_socket, 3);
    printf("Server is listening on port %d...\n", PORT);

    // Accept and handle client connections
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected.\n");
        handle_client(client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

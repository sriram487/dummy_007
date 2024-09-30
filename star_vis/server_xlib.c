#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define PORT 8080
#define BUFFER_SIZE (6 * 1024)
#define MAX_FLOATS (118 * 3)

Display *display;
Window window;
GC gc;

void init_xlib() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Unable to open X display\n");
        exit(1);
    }
    int screen = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 1400, 1400, 1,
                                  BlackPixel(display, screen), BlackPixel(display, screen)); // Black background
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);
    gc = XCreateGC(display, window, 0, NULL);
}

void draw_star_xlib(int x, int y, float magnitude, int ROI) {
    double H = 90000 * exp(-magnitude + 1);
    double sigma = 0.5;

    for (int u = x - ROI; u <= x + ROI; u++) {
        for (int v = y - ROI; v <= y + ROI; v++) {
            double dist = (u - x) * (u - x) + (v - y) * (v - y);
            double diff = dist / (2 * (sigma * sigma));
            double exponent_exp = exp(-diff);
            int raw_intensity = (int)round((H / (2 * M_PI * (sigma * sigma))) * exponent_exp);

            if (u >= 0 && u < 1400 && v >= 0 && v < 1400) {
                // Draw point if intensity is above 0
                if (raw_intensity > 0) {
                    XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
                    XDrawPoint(display, window, gc, u, v);
                }
            }
        }
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    char full_data[BUFFER_SIZE] = {0};
    ssize_t bytes_received;

    init_xlib();

    while (1) {
        const char *message = "READY\n";
        send(client_socket, message, strlen(message), 0);
        printf("Sent: %s", message);

        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Error or connection closed\n");
            break;
        }
        buffer[bytes_received] = '\0';

        printf("Received %ld bytes\n", bytes_received);

        if (buffer[0] == '$' && strchr(buffer, '&') != NULL) {
            char *end_ptr = strchr(buffer, '&');
            *end_ptr = '\0';
            strncat(full_data, buffer + 1, sizeof(full_data) - strlen(full_data) - 1);

            float float_array[MAX_FLOATS];
            size_t float_count = 0;

            for (size_t i = 0; i < strlen(full_data); i++) {
                if (full_data[i] == '\n') {
                    full_data[i] = ',';
                }
            }

            char *token = strtok(full_data, ",");
            while (token != NULL && float_count < MAX_FLOATS) {
                float_array[float_count++] = strtof(token, NULL);
                token = strtok(NULL, ",");
            }

            for (size_t i = 0; i < float_count; i += 3) {
                if (i + 2 < float_count) {
                    int x = (int)float_array[i];
                    int y = (int)float_array[i + 1];
                    float magnitude = float_array[i + 2];
                    draw_star_xlib(x, y, magnitude, 5);
                }
            }

            time_t start_time = time(NULL);


            XFlush(display); // Update the window

            time_t end_time = time(NULL);
            double elapsed = difftime(end_time, start_time);
            printf("Elapsed time: %.5f seconds\n", elapsed);

            const char *ack = "ACK: RD.\n";
            send(client_socket, ack, strlen(ack), 0);
            printf("Sent: %s", ack);
        } else {
            const char *ack = "ACK: Waiting for proper data format.\n";
            send(client_socket, ack, strlen(ack), 0);
            printf("Sent: %s", ack);
        }

        memset(full_data, 0, sizeof(full_data));
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    listen(server_socket, 3);
    printf("Server is listening on port %d...\n", PORT);

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

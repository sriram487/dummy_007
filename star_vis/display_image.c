#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace cv;

int main() {
    // Load the image from file in grayscale
    Mat image = imread("star_image_csv_c.png", IMREAD_GRAYSCALE);
    if (image.empty()) {
        fprintf(stderr, "Error loading image\n");
        return -1;
    }

    printf("%d\t", image.type());
    printf("%d\t", image.channels());

    // Check pixel values
    double minVal, maxVal;
    minMaxLoc(image, &minVal, &maxVal);
    printf("Min pixel value: %f, Max pixel value: %f\n", minVal, maxVal);

    // Connect to the X server
    xcb_connection_t *conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Error connecting to X server\n");
        return -1;
    }

    // Get the first screen
    const xcb_setup_t *setup = xcb_get_setup(conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = iter.data;

    // Create a window
    xcb_window_t window = xcb_generate_id(conn);
    uint32_t values[] = {screen->black_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS};
    
    xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root,
                      0, 0, 1400, 1400, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual,
                      XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
                      values);
    
    // Map (show) the window
    xcb_map_window(conn, window);
    xcb_flush(conn);

    // Create an XImage for grayscale (8 bits per pixel)
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Unable to open display\n");
        return -1;
    }

    int width = image.cols;
    int height = image.rows;

    // Create an XImage for grayscale (8 bits per pixel)
    XImage *ximage = XCreateImage(dpy,
                                   DefaultVisual(dpy, DefaultScreen(dpy)),
                                   //    8, // Depth for grayscale (8 bits)
                                   1,
                                   ZPixmap, 0,
                                   (char*)image.data,
                                   width, height,
                                   32, 0);

    // Event loop to display the image and handle events
    int running = 1; // Variable to control the event loop
    while (running) {
        // Wait for events
        xcb_generic_event_t *event;
        while ((event = xcb_wait_for_event(conn))) {
            switch (event->response_type & ~0x80) { // Mask out the highest bit
                case XCB_EXPOSE:
                    // Draw the image when exposed
                    XPutImage(dpy, window, DefaultGC(dpy, DefaultScreen(dpy)),
                              ximage, 0, 0, 0, 0, width, height);
                    break;
                case XCB_KEY_PRESS: {
                    xcb_key_press_event_t *key_event = (xcb_key_press_event_t*)event;
                    if (key_event->detail == 36) { // Keycode for Enter key is typically 36
                        running = 0; // Exit loop on Enter key press
                    }
                    break;
                }
                default:
                    break;
            }
            free(event); // Free event after use
        }
    }

    // Cleanup resources before exiting
    xcb_destroy_window(conn, window);
    xcb_disconnect(conn);
    
    return 0;
}

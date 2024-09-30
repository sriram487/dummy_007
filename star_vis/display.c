#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void draw_star(int x, int y, float magnitude, cv::Mat* background, int ROI) {
    double H = 90000 * exp(-magnitude + 1);
    double sigma = 0.5;

    for (int u = x - ROI; u <= x + ROI; u++) {
        for (int v = y - ROI; v <= y + ROI; v++) {
            double dist = (u - x) * (u - x) + (v - y) * (v - y);
            double diff = dist / (2 * (sigma * sigma));
            double exponent_exp = exp(-diff);
            int raw_intensity = (int)round((H / (2 * CV_PI * (sigma * sigma))) * exponent_exp);

            // Boundary check and set pixel intensity
            if (u >= 0 && u < background->cols && v >= 0 && v < background->rows) {
                (*background).at<uchar>(v, u) = (uchar)fmin(raw_intensity, 255);
            }
        }
    }
}

int main() {
    // Load star data from CSV
    FILE* file = fopen("./data/transfer_stars.csv", "r");
    if (!file) {
        fprintf(stderr, "Error opening file.\n");
        return -1;
    }

    float stars[1000][3]; // Assuming max 1000 stars
    int star_count = 0;

    // Parse CSV file
    while (fscanf(file, "%f,%f,%f\n", &stars[star_count][0], &stars[star_count][1], &stars[star_count][2]) == 3) {
        star_count++;
    }
    fclose(file);

    // Create a background image
    cv::Mat background = cv::Mat::zeros(1400, 1400, CV_8UC1);

    // Draw stars on the background
    for (int i = 0; i < star_count; i++) {
        int x = (int)stars[i][0];
        int y = (int)stars[i][1];
        float mag = stars[i][2];
        draw_star(x, y, mag, &background, 5);
    }

    // Save the output image
    cv::imwrite("star_image_csv_c.png", background);

    return 0;
}
